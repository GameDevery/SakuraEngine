# SakuraEngine GPUScene 内存布局设计

## 概述

GPUScene 系统采用分层数据布局设计，将组件数据分为两类：
- **Core Data**: 预分段的连续存储，所有实例的核心组件
- **Additional Data**: 页面管理的扩展存储，支持不同 Archetype 的可选组件

## Core Data 布局

Core Data 采用预分段的统一布局，所有实例都预留空间给所有核心组件类型，允许稀疏填充。

```
Core Data Buffer (连续内存)
┌─────────────────────────────────────────────────────────────┐
│                    Instance Stride                         │
├─────────────┬─────────────┬─────────────┬─────────────────┤
│ Transform   │   Color     │  Material   │    (padding)    │ Instance 0
│   (64B)     │   (16B)     │   (32B)     │      (16B)      │
├─────────────┼─────────────┼─────────────┼─────────────────┤
│ Transform   │   Color     │  Material   │    (padding)    │ Instance 1
│   (64B)     │   (16B)     │   (32B)     │      (16B)      │
├─────────────┼─────────────┼─────────────┼─────────────────┤
│ Transform   │   Color     │  Material   │    (padding)    │ Instance 2
│   (64B)     │   (16B)     │   (32B)     │      (16B)      │
├─────────────┼─────────────┼─────────────┼─────────────────┤
│     ...     │     ...     │     ...     │      ...        │ ...
└─────────────┴─────────────┴─────────────┴─────────────────┘

Component Offsets:
- Transform:  offset = 0
- Color:      offset = 64  
- Material:   offset = 80
Instance Stride: 128 bytes (aligned to 128B boundary)
```

### Core Data 特点
- **统一布局**: 所有实例使用相同的内存布局
- **稀疏支持**: 实例可以只使用部分组件，未使用的组件区域保持未定义
- **高性能访问**: GPU 可以通过简单的公式计算组件地址
- **缓存友好**: 相同组件的数据在内存中连续排列

### 寻址公式
```cpp
component_address = base_address + (instance_id * instance_stride) + component_offset
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
│               AudioSource (256B)                           │
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
- AudioSource (音频源)
- 其他特定 Archetype 才有的组件

这种设计在保证高性能的同时，也提供了足够的灵活性来处理复杂的 ECS 场景需求。