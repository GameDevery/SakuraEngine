# GPUScene 最终设计方案

## 核心设计理念

### 1. 同步驱动而非分配驱动
- **Entity 在 ECS World 中已存在**，GPUScene 只负责 ECS → GPU 的数据同步
- 查询所有含有 `GPUSceneInstance` 组件的 Archetype 并同步到 GPU Buffer
- 不再提供 AllocateInstance/FreeInstance 接口

### 2. 统一布局 + 稀疏数据策略
- **分配所有 core_data 的空间段**，保证 GPU 端布局完全一致
- 如果 Entity 只有 GPUSceneInstance 没有其他 CoreData，保留空间但不拷贝数据
- 允许数据段冗余，但换来了超高效的 GPU 访问性能

```cpp
// GPU Buffer 统一布局示例：
// Instance0: [Transform✓][Color✗][Emission✓] - 有 Transform 和 Emission，Color 空缺
// Instance1: [Transform✓][Color✓][Emission✗] - 有 Transform 和 Color，Emission 空缺
// Instance2: [Transform✓][Color✗][Emission✗] - 只有 Transform
//
// GPU 访问：buffer[instanceID * totalStride + componentOffset] - O(1) 直接访问
```

### 3. 精确的 SparseUpload 更新机制
- `RequireUpload(entity, component)` 标记需要更新的特定组件
- `ExecuteUpload()` 时批量处理，只上传真正变化的数据
- 减少 GPU 传输带宽，支持细粒度更新控制

### 4. 配置驱动的自动化管理
- 启动时注册所有组件类型 ID，不支持动态注册
- Resize 策略完全由配置驱动，在 ExecuteUpload 中自动执行
- 用户只需配置策略，系统自动处理内存管理

## 关键接口

```cpp
struct GPUScene final {
    // 核心接口
    void Initialize(CGPUDeviceId device, const GPUSceneConfig& config);
    void RequireUpload(skr::ecs::Entity entity, CPUTypeID component);
    void ExecuteUpload(skr::render_graph::RenderGraph* graph);
};

struct GPUSceneConfig {
    skr::ecs::World* world;
    
    // 类型映射（启动时固定）
    skr::Map<CPUTypeID, GPUComponentTypeID> core_data_types;
    skr::Map<CPUTypeID, GPUComponentTypeID> additional_data_types;
    
    // 自动 Resize 配置
    bool enable_auto_resize = true;
    float auto_resize_threshold = 0.9f;
    float resize_growth_factor = 1.5f;
};
```

## 核心优势

1. **超高效 GPU 访问**：统一布局 + O(1) 直接索引
2. **精确更新控制**：SparseUpload 只传输变化数据
3. **完美 ECS 对齐**：直接基于 ECS Archetype 工作
4. **自动化管理**：配置驱动，用户无需手动内存管理
5. **可预测性能**：内存布局和访问模式完全可预测

## 实现要点

1. **启动时**：根据 config 注册所有 core_data 和 additional_data 类型
2. **运行时**：查询带 GPUSceneInstance 的 Archetype，为每个分配完整的 core_data 空间
3. **更新时**：根据 RequireUpload 标记，SparseUpload 只更新变化的组件
4. **Resize 时**：在 ExecuteUpload 中根据配置自动执行扩容策略

这个设计在简化用户接口的同时，实现了极高的 GPU 访问效率和精确的更新控制。