# SakuraEngine GPUScene 内存布局设计

## 概述

GPUScene 系统采用分层数据布局设计，将组件数据分为两类：
- **Core Data**: 预分段的连续存储，所有实例的核心组件
- **Additional Data**: 页面管理的扩展存储，支持不同 Archetype 的可选组件

## Core Data 布局

Core Data 采用 SOA (Structure of Arrays) 布局，将不同组件的数据分段连续存储，所有实例预留相同组件空间，允许稀疏填充。

```
Core Data Buffer (SOA 布局)
┌─────────────────────────────────────────────────────────────┐
│                  Transform Segment                         │
│                     (64B × N)                              │
├─────────────┬─────────────┬─────────────┬─────────────────┤
│Transform[0] │Transform[1] │Transform[2] │    ...          │
│    64B      │    64B      │    64B      │                 │
└─────────────┴─────────────┴─────────────┴─────────────────┘

┌─────────────────────────────────────────────────────────────┐
│                    Color Segment                           │
│                     (16B × N)                              │
├─────────────┬─────────────┬─────────────┬─────────────────┤
│  Color[0]   │  Color[1]   │  Color[2]   │    ...          │
│    16B      │    16B      │    16B      │                 │
└─────────────┴─────────────┴─────────────┴─────────────────┘

┌─────────────────────────────────────────────────────────────┐
│                  Material Segment                          │
│                     (32B × N)                              │
├─────────────┬─────────────┬─────────────┬─────────────────┤
│Material[0]  │Material[1]  │Material[2]  │    ...          │
│    32B      │    32B      │    32B      │                 │
└─────────────┴─────────────┴─────────────┴─────────────────┘

Component Segments:
- Transform Segment:  base_offset = 0,        size = 64B × max_instances
- Color Segment:      base_offset = 64B × N,  size = 16B × max_instances  
- Material Segment:   base_offset = 80B × N,  size = 32B × max_instances
Total Buffer Size: (64 + 16 + 32) × max_instances = 112B × max_instances
```

### Core Data 特点
- **SOA 布局**: 相同组件的数据连续存储，极佳的缓存局部性
- **分段管理**: 每种组件类型占用独立的连续内存段
- **稀疏支持**: 实例可以只使用部分组件，未使用的槽位保持未定义
- **高性能访问**: GPU 可以高效地批量处理相同类型的组件数据
- **向量化友好**: 便于 GPU 向量化指令处理

### 寻址公式
```cpp
component_address = segment_base_offset + (instance_id * component_size)
```

## Additional Data 布局

Additional Data 采用页面管理的方式，支持不同 Archetype 的差异化组件需求。

```
Additional Data Buffer (分页管理)
┌─────────────────────────────────────────────────────────────┐
│                     Page 0 (16KB)                          │
├─────────────────┬─────────────────┬─────────────────────────┤
│  Archetype A    │  Archetype A    │  Archetype A            │
│  Emission (16B) │  Emission (16B) │  Emission (16B)         │
│  Entity 5       │  Entity 12      │  Entity 23              │
├─────────────────┼─────────────────┼─────────────────────────┤
│       ...       │       ...       │        ...              │
└─────────────────┴─────────────────┴─────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│                     Page 1 (16KB)                          │
├───────────────────────────────┬─────────────────────────────┤
│       Archetype B             │       Archetype B           │
│    PhysicsBody (128B)         │    PhysicsBody (128B)       │
│       Entity 8                │       Entity 15             │
├───────────────────────────────┼─────────────────────────────┤
│            ...                │            ...              │
└───────────────────────────────┴─────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│                     Page 2 (16KB)                          │
├─────────────────────────────────────────────────────────────┤
│                  Archetype C                               │
│               ParticleData (256B)                           │
│                  Entity 3                                  │
├─────────────────────────────────────────────────────────────┤
│                     ...                                     │
└─────────────────────────────────────────────────────────────┘

GPU Page Table (页表)
┌──────────┬──────────┬──────────┬──────────┐
│ Page 0   │ Page 1   │ Page 2   │   ...    │
├──────────┼──────────┼──────────┼──────────┤
│Offset:0KB│Offset:16K│Offset:32K│   ...    │
│Size: 16KB│Size: 16KB│Size: 16KB│   ...    │
│Count: 1024│Count: 128│Count: 64 │   ...    │
└──────────┴──────────┴──────────┴──────────┘
```

### Additional Data 特点
- **按 Archetype 分组**: 相同类型的组件聚集在一起
- **16KB 页面**: 优化 GPU 内存访问模式
- **动态分配**: 根据需要分配页面，支持不同大小的组件
- **页表索引**: GPU 通过页表快速定位数据

### 寻址方式
```cpp
// GPU 着色器中的访问模式
PageTableEntry page = page_table[page_id];
component_address = additional_data_base + page.byte_offset + entity_offset_in_page
```

## 两种布局的协作

### 实例索引系统
```cpp
struct GPUSceneInstance {
    GPUSceneInstanceID instance_index;     // Core Data 中的线性索引
    GPUSceneCustomIndex custom_index;      // 24位紧凑索引 (Archetype + LocalOffset)
};
```

### 访问模式对比

| 特性 | Core Data | Additional Data |
|------|-----------|-----------------|
| 访问方式 | 直接计算偏移 | 页表查找 |
| 内存效率 | 可能有浪费 | 按需分配 |
| 访问速度 | 最快 | 较快 |
| 适用场景 | 必备组件 | 可选组件 |
| 缓存友好性 | 极佳 | 良好 |

## 使用建议

### Core Data 适用组件
- Transform (变换矩阵)
- Color (基础颜色)
- Material ID (材质索引)
- 其他所有实例都需要的基础数据

### Additional Data 适用组件
- Emission (发光属性)
- PhysicsBody (物理体)
- ParticleData (音频源)
- 其他特定 Archetype 才有的组件

这种设计在保证高性能的同时，也提供了足够的灵活性来处理复杂的 ECS 场景需求。

## HugeBuffer 整体布局

GPUScene 在 GPU 端的完整内存布局，展示 Core Data 和 Additional Data 在 HugeBuffer 中的组织方式。

**示例假设**: 系统只注册了 Transform (64B)、Color (16B)、Material (32B) 三个核心组件，支持 100万个实例。

```
GPU HugeBuffer 整体布局 (2GB 显存分配)
┌─────────────────────────────────────────────────────────────────────────────┐
│                           Core Data Region                                 │
│                          (112MB 实际使用)                                   │
├─────────────────────────────────────────────────────────────────────────────┤
│                        Transform Segment                                   │
│                    (64B × 1M instances = 64MB)                             │
├─────────────────┬─────────────────┬─────────────────┬─────────────────────┤
│  Transform[0]   │  Transform[1]   │  Transform[2]   │       ...           │
│   4x4 Matrix    │   4x4 Matrix    │   4x4 Matrix    │   Transform[999999] │
├─────────────────┴─────────────────┴─────────────────┴─────────────────────┤
│                         Color Segment                                      │
│                    (16B × 1M instances = 16MB)                             │
├─────────────────┬─────────────────┬─────────────────┬─────────────────────┤
│    Color[0]     │    Color[1]     │    Color[2]     │       ...           │
│     RGBA        │     RGBA        │     RGBA        │    Color[999999]    │
├─────────────────┴─────────────────┴─────────────────┴─────────────────────┤
│                       Material Segment                                     │
│                    (32B × 1M instances = 32MB)                             │
├─────────────────┬─────────────────┬─────────────────┬─────────────────────┤
│   Material[0]   │   Material[1]   │   Material[2]   │       ...           │
│   Material ID   │   Material ID   │   Material ID   │  Material[999999]   │
│   + Properties  │   + Properties  │   + Properties  │   + Properties      │
└─────────────────┴─────────────────┴─────────────────┴─────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│                      Additional Data Region                                │
│                        (分配 64MB，少数多态数据)                            │
├─────────────────────────────────────────────────────────────────────────────┤
│ Page 0 (16KB)   │ Page 1 (16KB)   │ Page 2 (16KB)   │ Page 3 (16KB)       │
│ Archetype A     │ Archetype A     │ Archetype B     │ Archetype C         │
│ Emission (16B)  │ Emission (16B)  │ PhysicsBody     │ ParticleData         │
│ ~1000 entities  │ ~1000 entities  │ (128B)~128 ents │ (256B)~64 entities  │
├─────────────────┼─────────────────┼─────────────────┼─────────────────────┤
│    Free Pages   │    Free Pages   │    Free Pages   │    Free Pages       │
│                 │                 │                 │                     │
│                 │                 │                 │                     │
├─────────────────┴─────────────────┴─────────────────┴─────────────────────┤
│                         Free Space                                         │
│                   (未分配页面区域，1.9GB+ 可用)                             │
│           大部分实体只使用 Core Data，无需 Additional Data                   │
└─────────────────────────────────────────────────────────────────────────────┘

总布局: 2GB HugeBuffer
├── Core Data:        112MB (Transform+Color+Material，SOA 布局)  
├── Additional Data:  64MB  (少数多态组件，16KB 页面)
```

### 布局特点

#### 分区隔离
- **Core Data**: 112MB，三个组件段连续排列 (Transform 64MB + Color 16MB + Material 32MB)
- **Additional Data**: 64MB，少数多态组件的 16KB 页面池
- **Metadata**: 16MB，页表和管理信息  
- **Available**: 1.8GB，大量可用空间支持场景扩展

#### 内存效率和使用模式
- **大多数实体**：只使用 Core Data (Transform + Color + Material)，内存高效
- **少数特殊实体**：额外使用 Additional Data (如 Emission、PhysicsBody 等)
- **内存占用对比**：
  - 普通渲染实体：112B (仅 Core Data)
  - 发光实体：112B + 16B (Core + Emission)  
  - 物理实体：112B + 128B (Core + PhysicsBody)
- **SOA 优势**：批量处理时缓存局部性最优
- **稀疏特性**：大部分 Additional Data 页面保持未分配状态

#### GPU 访问模式
```hlsl
// GPU 着色器中的访问示例
// Core Data 访问 (直接计算)
float4x4 transform = CoreDataBuffer.Load<float4x4>(transform_base + instanceID * 64);
float4 color = CoreDataBuffer.Load<float4>(color_base + instanceID * 16);

// Additional Data 访问 (页表查找)
PageEntry page = PageTable[pageID];
float4 emission = AdditionalDataBuffer.Load<float4>(page.offset + localOffset);
```

这种宏观布局设计确保了：
1. **Core Data 的极高访问性能**：SOA 布局 + 直接寻址
2. **Additional Data 的稀疏高效**：仅占用少量空间，按需分配
3. **整体系统的可扩展性**：1.8GB 可用空间支持场景增长
4. **内存使用的经济性**：大多数实体只占用必需的 Core Data