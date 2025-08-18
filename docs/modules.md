### Window System

跨平台窗口管理、显示器检测和输入法（IME）支持系统。采用简洁的抽象设计，通过 SystemApp 统一管理窗口、显示器和 IME。当前基于 SDL3 实现，提供了完整的跨平台支持。

- @docs/engine_systems/window_system.md - 系统架构和使用指南
- @docs/engine_systems/window_system_implementation_notes.md - 实现经验和设计决策

### ECS

项目使用的 ECS 框架，有极高的缓存性能和并发调度能力。现阶段接口比较晦涩。

- @docs/core_systems/ecs_sugoi.md

### ResourceSystem

项目使用的资源系统，通过把打包后的资源（或者某些状况下原本的特殊格式资源例如 ini）读取到内存并反序列化、异步初始化至可用状态的系统。

- @docs/core_systems/resource_system.md

### CGPU

高性能跨平台图形 API 抽象层，提供了统一的 D3D12、Vulkan 和 Metal 接口。采用零开销 C 接口设计，支持 DirectStorage、Tiled Resource 等现代 GPU 特性。

- @docs/graphics/cgpu_design.md

### RenderGraph

项目使用的调度渲染用的 RenderGraph 系统，提供了声明式的渲染管线构建方式。系统自动管理资源生命周期、状态转换和内存优化，让开发者专注于渲染逻辑而非底层资源管理。

- @docs/graphics/render_graph.md


### 任务系统

高性能并行计算框架，支持 Fiber 任务模型。采用工作窃取调度器，提供丰富的并行编程原语。项目里也有对 C++20 协程模型的玩具型支持，在分析业务需求时请忽略 task2 这个不完善的系统。

- @docs/core_systems/task_system.md

### I/O 服务系统

提供高性能的异步文件访问能力，支持传统文件系统、DirectStorage 硬件加速以及直接到 GPU 内存的传输。采用组件化设计，支持批处理、压缩、优先级调度等高级特性。

- @docs/core_systems/io_service.md

### 内存管理系统

基于 Microsoft 的 mimalloc 构建的高性能内存管理系统，提供统一的分配接口、智能指针（RC/SP）、内存池以及完善的调试支持。

- @docs/core_systems/memory_management.md
