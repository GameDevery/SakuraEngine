# Additional Data 元数据驱动属性系统

## 概述
基于 Unity Hybrid Renderer V2 的设计理念，实现灵活的 GPU 属性访问系统。

## 核心设计

### 1. 元数据系统
```cpp
// 属性元数据（32位）
struct PropertyMetadata {
    uint32_t offset : 24;      // 页面池偏移（最大16MB）
    uint32_t elementSize : 7;  // 元素大小（最大127字节）
    uint32_t isInstance : 1;   // 0=共享, 1=实例化
};
```

### 2. GPU访问流程
- 单次元数据查询
- 根据类型选择路径（共享/实例）
- 直接缓冲区访问

### 3. 页面管理
- 利用现有 PagePoolAllocator
- 每个属性独立分配页面
- 支持动态增长和回收

## 实施方案

### Phase 1: 基础架构
1. 扩展 GPUScene 添加属性注册表
2. 实现 PropertyPageManager 管理页面分配
3. 创建 GPU 元数据缓冲区

### Phase 2: 数据更新
1. 修改 ScanGPUScene 收集属性更新
2. 实现批量更新机制
3. 集成到现有的 Upload 流程

### Phase 3: GPU访问
1. 创建统一的属性访问 HLSL 函数
2. 更新着色器使用新的访问模式
3. 优化元数据缓存

## 关键数据结构

### CPU端
```cpp
class PropertyRegistry {
    PropertyMetadata metadata[MAX_PROPERTIES];
    PageUserID pageUsers[MAX_PROPERTIES];
    skr::Map<InstanceID, uint32_t> compactIndices[MAX_PROPERTIES];
};

class PropertyPageManager {
    PagePoolAllocator* pagePool;
    uint32_t AllocatePropertySpace(PropertyID prop, uint32_t count);
    void UpdateInstances(PropertyID prop, ArchetypeChunk& chunk);
};
```

### GPU端
```hlsl
// 统一访问接口
template<typename T>
T GetProperty(uint instanceID, uint propertyID) {
    uint metadata = LoadMetadata(propertyID);
    uint offset = GetOffset(metadata, instanceID);
    return g_SceneData.Load<T>(offset);
}
```

## 优势
1. **灵活性**: 实例可有不同属性组合
2. **内存效率**: 只存储使用的数据
3. **访问性能**: 最少间接寻址
4. **扩展性**: 易于添加新属性

## 与现有系统集成
- 复用 PagePoolAllocator 的页面管理
- 保持 Core Data 的直接访问模式
- 兼容现有的 Upload 系统