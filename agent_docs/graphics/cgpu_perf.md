# NVIDIA Nsight Perf SDK vs AMD GPUPerfAPI 技术对比与统一API设计

## 1. 核心概念映射

### 1.1 层级结构对比

| 层级 | NVIDIA Nsight Perf | AMD GPUPerfAPI | 统一API (CGPU Perf) |
|------|-------------------|----------------|-------------------|
| **初始化** | NVPW_InitializeHost | GPA_Initialize | cgpu_perf_initialize |
| **设备上下文** | Device Index + Chip Name | GPA_ContextId | CGPUPerfContext |
| **会话管理** | Profiler Session / Sampler Session | GPA_SessionId | CGPUPerfSession |
| **命令流** | Pass Index + Range | GPA_CommandListId | CGPUPerfCommandList |
| **采样单元** | Range (自动ID) / Sample | GPA_SampleId (用户控制) | CGPUPerfSampleId (用户控制) |

### 1.2 采样模式映射

| 采样类型 | NVIDIA 实现 | AMD 实现 | 特点 |
|---------|------------|----------|------|
| **离散计数器** (DISCRETE_COUNTER) | Range Profiler (NVPA_ACTIVITY_KIND_PROFILER) | Discrete Counter Mode | 需要多pass，精确采样 |
| **流式计数器** (STREAMING_COUNTER) | GPU Periodic Sampler (NVPA_ACTIVITY_KIND_REALTIME_SAMPLED) | Streaming Counter Mode (SPM) | 单pass，实时采样 |
| **指令追踪** (INSTRUCTION_TRACE) | SASS Source Correlation | SQTT (Shader Queue Thread Trace) | 指令级分析 |

### 1.3 多Pass机制对比

#### NVIDIA Range Profiler
```cpp
// 使用 ConfigImage 和 PassGroups
NVPW_RawMetricsConfig_GetConfigImage() -> configImage
NVPW_RawMetricsConfig_GetNumPasses() -> numPasses
// 每个pass使用相同的range ID，硬件自动切换counter组
```

#### AMD GPUPerfAPI
```cpp
// 显式的Pass管理
GPA_GetPassCount() -> numPasses
GPA_BeginPass(passIndex)
// 每个pass需要重新记录相同的sample ID
```

#### 统一API设计
```cpp
// 抽象的Pass接口
cgpu_perf_get_pass_count() -> 获取需要的pass数量
cgpu_perf_begin_command_list(pass_index) -> 开始特定pass的命令列表
// Sample ID由用户控制，跨pass保持一致
```

## 2. 关键技术差异

### 2.1 Sample ID 管理

**NVIDIA**: 
- Range Profiler使用自动分配的range索引
- 通过 PushRange/PopRange 管理嵌套
- DecodeCounters时通过range名称关联

**AMD**:
- Sample ID完全由用户控制
- 不支持嵌套采样
- 直接通过Sample ID查询结果

**统一方案**:
```cpp
// 采用AMD模式：用户控制的Sample ID
struct CGPUPerfSession_NV {
    // 映射用户Sample ID到NVIDIA内部range
    skr::HashMap<CGPUPerfSampleId, skr::String> sampleNames;
    skr::HashMap<CGPUPerfSampleId, SampleResult> sampleResults;
};
```

### 2.2 实时采样解决方案

**问题**: 多pass机制不适合内嵌的游戏内性能分析

**NVIDIA解决方案**:
```cpp
// 使用GPU Periodic Sampler避免多pass
NVPA_ACTIVITY_KIND_REALTIME_SAMPLED
// 支持的触发源
NVPW_GPU_PERIODIC_SAMPLER_TRIGGER_SOURCE_GPU_TIME_INTERVAL
```

**AMD解决方案**:
```cpp
// 使用SPM (Streaming Performance Monitor)
GPA_COUNTER_SAMPLE_TYPE_STREAMING
// 连续采样，无需多pass
```

### 2.3 Counter配置差异

| 方面 | NVIDIA | AMD |
|-----|--------|-----|
| **Counter枚举** | MetricsEvaluator + Metric Names | GPA_GetCounterName/Index |
| **Counter类型** | Metric (derived) + Raw Counter | Public/Hardware/Software counters |
| **配置方式** | RawMetricsConfig + ConfigImage | EnableCounter + BeginSession |
| **硬件限制** | 通过PassGroups自动处理 | 显式的Pass管理 |

## 3. 实现策略

### 3.1 初始化流程

```cpp
// NVIDIA实现
ECGPUPerfStatus cgpu_perf_initialize() {
    NVPW_InitializeHost();
    NVPW_D3D12_LoadDriver(); // 或 NVPW_Vulkan_LoadDriver
}

// AMD实现
ECGPUPerfStatus cgpu_perf_initialize() {
    GPA_Initialize(GPA_INITIALIZE_DEFAULT_BIT);
}
```

### 3.2 Context管理

```cpp
// NVIDIA - 需要获取device index和chip name
NVPW_D3D12_Device_GetDeviceIndex() -> deviceIndex
NVPW_Device_GetNames() -> chipName
CreateMetricsEvaluator() -> 创建counter查询接口

// AMD - 直接从API context创建
GPA_OpenContext(d3d12Device) -> contextId
```

### 3.3 Session配置

```cpp
struct CGPUPerfSession_NV {
    // Range Profiler状态 (DISCRETE_COUNTER)
    struct RangeProfilerState {
        NVPW_RawMetricsConfig* pRawMetricsConfig;
        vector<uint8_t> configImage;  // counter配置
        uint32_t numPasses;            // 多pass数量
        uint32_t currentPass;          // 当前pass索引
    };
    
    // Periodic Sampler状态 (STREAMING_COUNTER)
    struct PeriodicSamplerState {
        vector<uint8_t> recordBuffer;  // 采样缓冲区
        bool isRunning;
    };
};
```

### 3.4 Sample映射实现

```cpp
// 将用户的sample ID映射到NVIDIA的range
ECGPUPerfStatus cgpu_perf_begin_sample(CGPUPerfSampleId sample_id, 
                                       CGPUPerfCommandListId cmd) {
    auto* cmdList = static_cast<CGPUPerfCommandList_NV*>(cmd);
    auto* session = static_cast<CGPUPerfSession_NV*>(cmdList->session);
    
    // 生成range名称
    char rangeName[256];
    sprintf(rangeName, "Sample_%u", sample_id);
    
    // NVIDIA: PushRange
    NVPW_D3D12_Profiler_PushRange_Params params;
    params.pCommandList = cmdList->pD3D12CommandList;
    params.pRangeName = rangeName;
    NVPW_D3D12_Profiler_PushRange(&params);
    
    // 记录映射
    session->sampleNames[sample_id] = rangeName;
    cmdList->currentSampleId = sample_id;
}
```

### 3.5 多Pass处理

```cpp
// 获取pass数量
ECGPUPerfStatus cgpu_perf_get_pass_count(CGPUPerfSessionId session, 
                                         uint32_t* num_passes) {
    auto* sess = static_cast<CGPUPerfSession_NV*>(session);
    
    if (sess->sampleType == DISCRETE_COUNTER) {
        // 从ConfigImage获取
        NVPW_RawMetricsConfig_GetNumPasses_Params params;
        params.pRawMetricsConfig = sess->rangeProfiler.pRawMetricsConfig;
        NVPW_RawMetricsConfig_GetNumPasses(&params);
        *num_passes = params.numPasses;
    } else {
        *num_passes = 1; // Streaming只需要一个pass
    }
}

// 检查pass完成状态
ECGPUPerfStatus cgpu_perf_is_pass_complete(CGPUPerfSessionId session,
                                           uint32_t pass_index) {
    auto* sess = static_cast<CGPUPerfSession_NV*>(session);
    
    if (pass_index < sess->rangeProfiler.passCompleted.size() &&
        sess->rangeProfiler.passCompleted[pass_index]) {
        return CGPU_PERF_STATUS_OK;
    }
    return CGPU_PERF_STATUS_RESULT_NOT_READY;
}
```

## 4. 最佳实践建议

### 4.1 采样模式选择

| 场景 | 推荐模式 | 原因 |
|------|---------|------|
| **游戏内HUD** | STREAMING_COUNTER | 避免多pass，实时性好 |
| **离线分析** | DISCRETE_COUNTER | 精度高，counter覆盖全 |
| **调试分析** | INSTRUCTION_TRACE | 指令级精度 |

### 4.2 性能优化建议

1. **减少Counter数量**: 只启用必要的counter减少pass数量
2. **批量采样**: 使用command list批量提交采样命令
3. **异步解码**: 在单独线程进行counter数据解码
4. **缓存配置**: 重用ConfigImage避免重复计算

### 4.3 错误处理

```cpp
// 统一的错误码转换
static ECGPUPerfStatus ConvertFromNvStatus(NVPA_Status status) {
    switch(status) {
        case NVPA_STATUS_SUCCESS: 
            return CGPU_PERF_STATUS_OK;
        case NVPA_STATUS_NOT_SUPPORTED:
            return CGPU_PERF_STATUS_ERROR_HARDWARE_NOT_SUPPORTED;
        case NVPA_STATUS_NOT_READY:
            return CGPU_PERF_STATUS_RESULT_NOT_READY;
        default:
            return CGPU_PERF_STATUS_ERROR_FAILED;
    }
}
```

## 5. 实现清单

```cpp
// 创建Metrics评估器
static ECGPUPerfStatus CreateMetricsEvaluator(CGPUPerfContext_NV* ctx) {
    NVPW_CUDA_MetricsEvaluator_CalculateScratchBufferSize_Params scratchParams;
    scratchParams.pChipName = ctx->chipName.c_str();
    NVPW_CUDA_MetricsEvaluator_CalculateScratchBufferSize(&scratchParams);
    
    ctx->metricsEvaluatorScratch.resize(scratchParams.scratchBufferSize);
    
    NVPW_CUDA_MetricsEvaluator_Initialize_Params initParams;
    initParams.scratchBufferSize = ctx->metricsEvaluatorScratch.size();
    initParams.pScratchBuffer = ctx->metricsEvaluatorScratch.data();
    initParams.pChipName = ctx->chipName.c_str();
    NVPW_CUDA_MetricsEvaluator_Initialize(&initParams);
    
    ctx->pMetricsEvaluator = initParams.pMetricsEvaluator;
}

// 构建Counter配置
static ECGPUPerfStatus BuildCounterConfiguration(CGPUPerfSession_NV* session) {
    // 将启用的counter转换为metric names
    vector<const char*> metricNames;
    for (auto index : session->enabledCounters) {
        metricNames.push_back(ctx->counters[index].name.c_str());
    }
    
    // 设置请求的metrics
    NVPW_RawMetricsConfig_SetMetrics_Params setMetricsParams;
    setMetricsParams.pRawMetricsConfig = session->rangeProfiler.pRawMetricsConfig;
    setMetricsParams.ppMetricNames = metricNames.data();
    setMetricsParams.numMetricNames = metricNames.size();
    NVPW_RawMetricsConfig_SetMetrics(&setMetricsParams);
    
    // 生成ConfigImage
    NVPW_RawMetricsConfig_GenerateConfigImage_Params genParams;
    genParams.pRawMetricsConfig = session->rangeProfiler.pRawMetricsConfig;
    NVPW_RawMetricsConfig_GenerateConfigImage(&genParams);
    
    // 获取pass数量
    NVPW_RawMetricsConfig_GetNumPasses_Params passParams;
    passParams.pRawMetricsConfig = session->rangeProfiler.pRawMetricsConfig;
    NVPW_RawMetricsConfig_GetNumPasses(&passParams);
    session->rangeProfiler.numPasses = passParams.numPasses;
}
```

## 6. 流程

```cpp
// 初始化
cgpu_perf_initialize(CGPU_PERF_INITIALIZE_DEFAULT_BIT);

// 打开context
CGPUPerfContextId context;
cgpu_perf_open_context(device, 0, &context);

// 创建session
CGPUPerfSessionId session;
cgpu_perf_create_session(context, queue, DISCRETE_COUNTER, "test", &session);

// 启用counter
cgpu_perf_enable_counter_by_name(session, "sm__throughput.avg.pct_of_peak_sustained_elapsed");

// 获取pass数量
uint32_t numPasses;
cgpu_perf_get_pass_count(session, &numPasses);

// 执行采样
for (uint32_t pass = 0; pass < numPasses; pass++) {
    CGPUPerfCommandListId cmdList;
    cgpu_perf_begin_command_list(session, pass, cmd, PRIMARY, &cmdList);
    
    cgpu_perf_begin_sample(0, cmdList);
    // ... 渲染命令 ...
    cgpu_perf_end_sample(cmdList);
    
    cgpu_perf_end_command_list(cmdList);
}

// 解码和查询结果
cgpu_perf_decode_samples(session);
double result;
cgpu_perf_get_sample_result_as_float64(0, counterIndex, &result);
```

## 7. Instruction Trace模式详解

### 7.1 概述
Instruction Trace（指令追踪）模式是GPU性能分析的最深层级，提供指令级的执行追踪。与计数器模式不同，Instruction Trace记录每条GPU指令的详细执行信息，包括执行时序、停顿原因和内存访问模式。

### 7.2 AMD SQTT (Shader Queue Thread Trace)

#### 7.2.1 采样类型标志
```cpp
// AMD GPUPerfAPI中的SQTT相关标志
typedef enum GPA_Session_Sample_Type_Bits
{
    // 基础SQTT模式
    kGpaSessionSampleTypeSqtt = 0x04,
    
    // 混合模式：同时采集流式计数器和SQTT
    kGpaSessionSampleTypeStreamingCounterAndSqtt = 0x08,
    
} GPA_Session_Sample_Type_Bits;
```

#### 7.2.2 SQTT数据结构
```cpp
// SQTT记录的指令信息
struct SqttInstruction {
    uint64_t timestamp;        // GPU时钟周期
    uint32_t pc;              // 程序计数器
    uint32_t exec_mask;       // 执行掩码(哪些线程活跃)
    uint32_t wave_id;         // Wavefront ID
    uint32_t simd_id;         // SIMD单元ID
    uint32_t cu_id;           // Compute Unit ID
    
    // 指令类型
    enum InstructionType {
        VALU,    // Vector ALU
        SALU,    // Scalar ALU  
        VMEM,    // Vector Memory
        SMEM,    // Scalar Memory
        LDS,     // Local Data Share
        GDS,     // Global Data Share
        EXPORT,  // Export/Pixel输出
        BRANCH   // 分支指令
    } type;
    
    // 停顿信息
    struct StallInfo {
        bool wait_vmem;       // 等待内存
        bool wait_lgkm;       // 等待LDS/GDS/常量
        bool wait_exp;        // 等待Export
        bool wait_valu;       // 等待ALU
        uint32_t cycles;      // 停顿周期数
    } stall;
};
```

#### 7.2.3 使用SQTT的API流程
```cpp
// 1. 创建SQTT session
GPA_CreateSession(context, 
    kGpaSessionSampleTypeSqtt,  // 纯SQTT模式
    &sqtt_session);

// 2. 配置SQTT参数
GPA_SPM_SetSqttInstructionMask(
    0xFFFFFFFF,  // 捕获所有指令类型
    true         // 包含时序信息
);

// 3. 设置内存缓冲区大小
size_t buffer_size = 256 * 1024 * 1024; // 256MB
GPA_SetSqttBufferSize(sqtt_session, buffer_size);

// 4. 执行采样
GPA_BeginCommandList(sqtt_session, cmd_list);
GPA_BeginSample(sample_id, cmd_list);
// ... GPU工作负载 ...
GPA_EndSample(cmd_list);
GPA_EndCommandList(cmd_list);

// 5. 解析SQTT数据
void* sqtt_data;
size_t data_size;
GPA_GetSqttData(sample_id, &sqtt_data, &data_size);
```

### 7.3 NVIDIA SASS追踪

#### 7.3.1 SASS (Streaming Assembly)配置
```cpp
// NVIDIA的指令追踪配置
NVPW_RawMetricsConfig_SetActivityKind(
    config,
    NVPA_ACTIVITY_KIND_SOURCE_LEVEL_SASS  // SASS源码级追踪
);

// 配置源码关联
NVPW_RawMetricsConfig_SetSourceLocations(
    config,
    true  // 启用源码位置映射
);
```

#### 7.3.2 SASS指令数据
```cpp
struct SassTraceRecord {
    // 基本信息
    uint64_t timestamp;       // GPU时钟
    uint32_t sm_id;          // SM ID
    uint32_t warp_id;        // Warp ID (32线程组)
    uint64_t pc;             // 程序计数器
    uint32_t active_mask;    // 活跃线程掩码
    
    // 指令信息
    char opcode[32];         // 指令操作码 (如 "LDG.E", "FFMA")
    uint32_t latency;        // 指令延迟
    
    // 内存访问信息
    struct MemoryAccess {
        uint64_t address;    // 访问地址
        uint32_t size;       // 访问大小
        bool coalesced;      // 是否合并访问
        uint32_t bank_conflicts; // Bank冲突数
    } mem_access;
    
    // 停顿原因
    enum StallReason {
        STALL_NONE = 0,
        STALL_MEMORY = 1,        // 内存延迟
        STALL_TEXTURE = 2,       // 纹理缓存未命中
        STALL_DATA_DEPENDENCY = 3, // 数据依赖
        STALL_SYNC = 4,          // 同步等待
        STALL_BRANCH = 5,        // 分支发散
        STALL_NOT_SELECTED = 6   // 未被调度
    } stall_reason;
};
```

### 7.4 混合模式 (AMD特有)

#### 7.4.1 StreamingCounterAndSqtt模式
```cpp
// 同时启用流式计数器和SQTT
GPA_CreateSession(context,
    kGpaSessionSampleTypeStreamingCounterAndSqtt,
    &hybrid_session);

// 优势：
// 1. 一次采样获得性能指标和指令细节
// 2. 可以关联性能瓶颈到具体指令
// 3. 减少多次profiling的开销

// 配置示例
struct HybridConfig {
    // SPM配置
    uint32_t sampling_interval_ns = 1000;  // 1μs采样间隔
    uint32_t spm_counters[16];             // SPM计数器选择
    
    // SQTT配置  
    uint32_t sqtt_buffer_size = 128 * 1024 * 1024;
    uint32_t sqtt_mask = 0xFFFFFFFF;
    bool sqtt_timing = true;
};
```

#### 7.4.2 数据关联分析
```cpp
// 将SPM计数器数据与SQTT指令关联
struct CorrelatedData {
    // 时间窗口内的性能指标
    struct PerfMetrics {
        float sm_occupancy;      // SM占用率
        float memory_bandwidth;  // 内存带宽
        float l2_cache_hit_rate; // L2缓存命中率
    } metrics;
    
    // 同时间窗口的热点指令
    vector<SqttInstruction> hot_instructions;
    
    // 分析结果
    struct Analysis {
        bool memory_bound;       // 内存瓶颈
        bool compute_bound;      // 计算瓶颈
        float divergence_rate;   // 分支发散率
    } analysis;
};
```

### 7.5 Instruction Trace的实际应用

#### 7.5.1 Shader优化示例
```hlsl
// 原始HLSL代码
[numthreads(256, 1, 1)]
void MainCS(uint3 id : SV_DispatchThreadID) {
    float4 data = Buffer[id.x];
    data = data * 2.0f + 1.0f;
    OutputBuffer[id.x] = data;
}
```

#### 7.5.2 SQTT分析结果
```asm
// AMD GCN汇编追踪
PC    Cycles  Instruction                    Stall
0x00  1       s_buffer_load_dwordx4 s[0:3]  -
0x04  400     s_waitcnt lgkmcnt(0)           WAIT_LGKM  // 内存延迟
0x08  1       v_mul_f32 v0, v0, 2.0          -
0x0C  1       v_add_f32 v0, v0, 1.0          -
0x10  1       v_mul_f32 v1, v1, 2.0          -
0x14  1       v_add_f32 v1, v1, 1.0          -
0x18  1       buffer_store_dwordx4           -

// 分析：内存加载延迟是主要瓶颈
```

#### 7.5.3 优化后的代码
```hlsl
// 优化：预取和流水线化
[numthreads(256, 1, 1)]
void MainCS(uint3 id : SV_DispatchThreadID) {
    // 预取下一个数据
    float4 data0 = Buffer[id.x * 2];
    float4 data1 = Buffer[id.x * 2 + 1];
    
    // 交错计算，隐藏延迟
    data0 = data0 * 2.0f;
    data1 = data1 * 2.0f;
    data0 = data0 + 1.0f;
    data1 = data1 + 1.0f;
    
    OutputBuffer[id.x * 2] = data0;
    OutputBuffer[id.x * 2 + 1] = data1;
}
```

### 7.6 统一API中的Instruction Trace实现

```cpp
// 统一API的Instruction Trace session创建
ECGPUPerfStatus cgpu_perf_create_trace_session(
    CGPUPerfContextId context,
    CGPUQueueId queue,
    CGPUPerfTraceConfig* config,
    CGPUPerfSessionId* session)
{
    auto* ctx = static_cast<CGPUPerfContext_NV*>(context);
    
#ifdef NVIDIA_GPU
    // NVIDIA: 配置SASS追踪
    NVPW_RawMetricsConfig_Create_Params createParams = {};
    createParams.activityKind = NVPA_ACTIVITY_KIND_SOURCE_LEVEL_SASS;
    createParams.pChipName = ctx->chipName.c_str();
    NVPW_RawMetricsConfig_Create(&createParams);
    
#elif AMD_GPU
    // AMD: 配置SQTT
    GPA_Session_Sample_Type sample_type = 
        config->enable_counters ? 
        kGpaSessionSampleTypeStreamingCounterAndSqtt :
        kGpaSessionSampleTypeSqtt;
    
    GPA_CreateSession(ctx->gpa_context, sample_type, session);
    GPA_SetSqttBufferSize(*session, config->buffer_size);
#endif
    
    return CGPU_PERF_STATUS_OK;
}
```

### 7.7 性能影响和限制

| 模式 | 性能开销 | 内存使用 | 典型用途 |
|-----|---------|----------|---------|
| **纯SQTT/SASS** | 50-70% | 128-512MB | 单个kernel深度分析 |
| **StreamingCounter** | 1-3% | 10-50MB | 实时监控 |
| **混合模式** | 60-80% | 256MB-1GB | 综合性能分析 |

### 7.8 最佳实践

1. **分阶段分析**：
   - 先用DISCRETE_COUNTER找到热点kernel
   - 再用INSTRUCTION_TRACE深入分析具体kernel
   
2. **缓冲区管理**：
   ```cpp
   // 根据工作负载调整缓冲区大小
   size_t EstimateTraceBufferSize(uint32_t num_waves, 
                                  uint32_t avg_instructions) {
       // 每条指令约32字节
       return num_waves * avg_instructions * 32;
   }
   ```

3. **数据过滤**：
   ```cpp
   // 只追踪特定shader或kernel
   cgpu_perf_set_trace_filter(session, "MyComputeShader");
   ```

## 8. Instruction Trace API使用详解与周期分析

### 8.1 AMD GPUPerfAPI - SQTT指令周期分析

#### 8.1.1 完整的SQTT采样流程
```cpp
// 1. 初始化GPUPerfAPI
GPA_Status status = GPA_Initialize(GPA_INITIALIZE_DEFAULT_BIT);

// 2. 打开Context并创建SQTT Session
GPA_ContextId context;
GPA_OpenContext(d3d12_device, GPA_OPENCONTEXT_DEFAULT_BIT, &context);

GPA_SessionId sqtt_session;
GPA_CreateSession(context, 
    GPA_SESSION_SAMPLE_TYPE_SQTT,  // 指令追踪模式
    &sqtt_session);

// 3. 配置SQTT详细参数
GPA_SQTTConfiguration sqtt_config = {};
sqtt_config.buffer_size = 256 * 1024 * 1024;  // 256MB缓冲区
sqtt_config.timing_enabled = true;            // 启用时序信息
sqtt_config.instruction_mask = 0xFFFFFFFF;    // 捕获所有指令类型
sqtt_config.compute_unit_mask = 0xFFFFFFFF;   // 所有CU
GPA_SetSQTTConfiguration(sqtt_session, &sqtt_config);

// 4. 开始Session
GPA_BeginSession(sqtt_session);

// 5. 录制命令并采样
GPA_CommandListId cmd_list;
GPA_BeginCommandList(sqtt_session, 
    0,  // pass index (SQTT只需要1个pass)
    d3d12_command_list, 
    GPA_COMMAND_LIST_PRIMARY,
    &cmd_list);

// 开始采样
GPA_BeginSample(0, cmd_list);  // sample_id = 0

// 执行GPU工作负载
dispatch_compute_shader();

// 结束采样
GPA_EndSample(cmd_list);
GPA_EndCommandList(cmd_list);

// 6. 结束Session并等待数据
GPA_EndSession(sqtt_session);
GPA_IsSessionReady(sqtt_session);  // 轮询直到准备好

// 7. 获取SQTT原始数据
size_t sqtt_data_size;
GPA_GetSQTTDataSize(sqtt_session, 0, &sqtt_data_size);

void* sqtt_data = malloc(sqtt_data_size);
GPA_GetSQTTData(sqtt_session, 0, sqtt_data);
```

#### 8.1.2 SQTT数据解析与周期计算
```cpp
// SQTT数据格式（简化版）
struct SQTTPacket {
    uint32_t packet_type : 4;   // 包类型
    uint32_t reserved : 28;
    uint64_t timestamp;         // GPU时钟周期
};

// 指令执行包
struct SQTTInstructionPacket : SQTTPacket {
    uint32_t pc;               // 程序计数器
    uint32_t wave_id;          // Wavefront ID
    uint32_t cu_id;            // Compute Unit ID
    uint32_t exec_mask;        // 执行掩码
    uint32_t instruction_type; // VALU/SALU/VMEM等
    uint32_t stall_cycles;     // 停顿周期数
};

// 解析SQTT数据流
class SQTTParser {
public:
    struct InstructionTiming {
        uint64_t start_cycle;   // 开始执行周期
        uint64_t end_cycle;     // 结束执行周期
        uint32_t stall_cycles;  // 停顿周期
        uint32_t active_cycles; // 活跃执行周期
        
        uint32_t GetTotalCycles() const {
            return end_cycle - start_cycle;
        }
        
        float GetEfficiency() const {
            return (float)active_cycles / GetTotalCycles();
        }
    };
    
    std::map<uint32_t, InstructionTiming> ParseTimings(void* sqtt_data, size_t size) {
        std::map<uint32_t, InstructionTiming> timings;  // PC -> Timing
        
        uint8_t* data = (uint8_t*)sqtt_data;
        size_t offset = 0;
        
        while (offset < size) {
            SQTTPacket* packet = (SQTTPacket*)(data + offset);
            
            if (packet->packet_type == SQTT_PACKET_INSTRUCTION) {
                auto* inst_packet = (SQTTInstructionPacket*)packet;
                
                // 记录或更新指令时序
                if (timings.find(inst_packet->pc) == timings.end()) {
                    timings[inst_packet->pc].start_cycle = packet->timestamp;
                }
                
                auto& timing = timings[inst_packet->pc];
                timing.end_cycle = packet->timestamp;
                timing.stall_cycles += inst_packet->stall_cycles;
                timing.active_cycles = timing.GetTotalCycles() - timing.stall_cycles;
            }
            
            offset += GetPacketSize(packet->packet_type);
        }
        
        return timings;
    }
    
    // 分析热点指令
    std::vector<HotInstruction> FindHotspots(
        const std::map<uint32_t, InstructionTiming>& timings,
        int top_n = 10) 
    {
        std::vector<HotInstruction> hotspots;
        
        for (const auto& [pc, timing] : timings) {
            hotspots.push_back({
                .pc = pc,
                .total_cycles = timing.GetTotalCycles(),
                .stall_cycles = timing.stall_cycles,
                .efficiency = timing.GetEfficiency()
            });
        }
        
        // 按总周期数排序
        std::sort(hotspots.begin(), hotspots.end(), 
            [](const auto& a, const auto& b) {
                return a.total_cycles > b.total_cycles;
            });
        
        hotspots.resize(std::min(hotspots.size(), (size_t)top_n));
        return hotspots;
    }
};
```

#### 8.1.3 停顿原因分析
```cpp
// AMD GPU停顿原因详解
enum SQTTStallReason {
    SQTT_STALL_NONE = 0,
    
    // 内存相关
    SQTT_STALL_VMEM = 1,      // Vector Memory等待
    SQTT_STALL_SMEM = 2,      // Scalar Memory等待
    SQTT_STALL_LDS = 3,       // Local Data Share等待
    SQTT_STALL_GDS = 4,       // Global Data Share等待
    
    // 执行单元相关
    SQTT_STALL_VALU = 5,      // Vector ALU忙
    SQTT_STALL_SALU = 6,      // Scalar ALU忙
    
    // 同步相关
    SQTT_STALL_BARRIER = 7,   // Barrier同步等待
    SQTT_STALL_SIGNAL = 8,    // Signal等待
    
    // 数据依赖
    SQTT_STALL_DEP_VGPR = 9,  // VGPR依赖
    SQTT_STALL_DEP_SGPR = 10, // SGPR依赖
    
    // 调度相关
    SQTT_STALL_ARBITER = 11,  // 仲裁器等待
    SQTT_STALL_SLEEP = 12     // Sleep指令
};

// 分析停顿分布
struct StallAnalysis {
    std::map<SQTTStallReason, uint64_t> stall_distribution;
    uint64_t total_stall_cycles;
    
    void AnalyzeStalls(const SQTTData* data) {
        // 遍历所有停顿事件
        for (const auto& stall_event : data->stall_events) {
            stall_distribution[stall_event.reason] += stall_event.cycles;
            total_stall_cycles += stall_event.cycles;
        }
    }
    
    void PrintReport() {
        printf("Stall Analysis Report:\n");
        printf("Total Stall Cycles: %llu\n", total_stall_cycles);
        printf("\nStall Distribution:\n");
        
        for (const auto& [reason, cycles] : stall_distribution) {
            float percentage = (float)cycles / total_stall_cycles * 100.0f;
            printf("  %s: %llu cycles (%.2f%%)\n", 
                GetStallReasonName(reason), cycles, percentage);
        }
    }
};
```

### 8.2 NVIDIA Nsight Perf - SASS指令周期分析

#### 8.2.1 NVIDIA SASS追踪设置
```cpp
// 1. 创建支持SASS追踪的RawMetricsConfig
NVPW_RawMetricsConfig* CreateSASSConfig(const char* chip_name) {
    NVPW_D3D12_RawMetricsConfig_Create_Params createParams = {
        NVPW_D3D12_RawMetricsConfig_Create_Params_STRUCT_SIZE
    };
    createParams.activityKind = NVPA_ACTIVITY_KIND_SOURCE_LEVEL_SASS;
    createParams.pChipName = chip_name;
    
    NVPW_D3D12_RawMetricsConfig_Create(&createParams);
    
    // 配置SASS源码关联
    NVPW_RawMetricsConfig_SetSourceLocations_Params srcParams = {
        NVPW_RawMetricsConfig_SetSourceLocations_Params_STRUCT_SIZE
    };
    srcParams.pRawMetricsConfig = createParams.pRawMetricsConfig;
    srcParams.bIncludeLocationInfo = true;
    srcParams.bIncludeInstructionInfo = true;
    NVPW_RawMetricsConfig_SetSourceLocations(&srcParams);
    
    return createParams.pRawMetricsConfig;
}

// 2. 设置SASS性能计数器
void ConfigureSASSMetrics(NVPW_RawMetricsConfig* config) {
    // 关键的SASS性能指标
    const char* sass_metrics[] = {
        "smsp__inst_executed.sum",           // 执行的指令数
        "smsp__inst_executed.sum.per_cycle", // 每周期指令数(IPC)
        "smsp__warps_active.avg",            // 活跃warp数
        "smsp__issue_active.avg.pct_of_peak_sustained_active", // 发射效率
        "sm__inst_executed_pipe_lsu.avg.pct_of_peak_sustained_active", // LSU利用率
        "sm__inst_executed_pipe_alu.avg.pct_of_peak_sustained_active", // ALU利用率
        "smsp__warp_issue_stalled_barrier_not_reached.avg.pct_of_peak_sustained_active", // Barrier停顿
        "smsp__warp_issue_stalled_long_scoreboard_not_ready.avg.pct_of_peak_sustained_active", // 长延迟停顿
        "smsp__warp_issue_stalled_short_scoreboard_not_ready.avg.pct_of_peak_sustained_active", // 短延迟停顿
        "smsp__warp_issue_stalled_membar_not_ready.avg.pct_of_peak_sustained_active", // 内存屏障停顿
    };
    
    NVPW_RawMetricsConfig_SetMetrics_Params setMetricsParams = {
        NVPW_RawMetricsConfig_SetMetrics_Params_STRUCT_SIZE
    };
    setMetricsParams.pRawMetricsConfig = config;
    setMetricsParams.ppMetricNames = sass_metrics;
    setMetricsParams.numMetricNames = sizeof(sass_metrics) / sizeof(sass_metrics[0]);
    
    NVPW_RawMetricsConfig_SetMetrics(&setMetricsParams);
}
```

#### 8.2.2 SASS指令周期分析
```cpp
class SASSAnalyzer {
public:
    // SASS指令信息
    struct SASSInstruction {
        uint64_t pc;               // 程序计数器
        std::string opcode;        // 操作码 (如 "LDG", "FFMA")
        uint32_t latency;          // 指令延迟
        uint32_t throughput;       // 吞吐率（周期/指令）
        bool is_memory_op;         // 是否为内存操作
        bool is_control_flow;      // 是否为控制流
    };
    
    // 分析SASS指令执行
    struct InstructionProfile {
        uint64_t execution_count;  // 执行次数
        uint64_t total_cycles;     // 总周期数
        uint64_t stall_cycles;     // 停顿周期
        
        // 停顿原因分解
        struct StallBreakdown {
            uint64_t memory_dependency;     // 内存依赖
            uint64_t execution_dependency;  // 执行依赖
            uint64_t texture_dependency;    // 纹理依赖
            uint64_t sync_dependency;       // 同步依赖
            uint64_t not_selected;          // 未被调度
            uint64_t misc;                  // 其他
        } stalls;
        
        double GetAverageCycles() const {
            return (double)total_cycles / execution_count;
        }
        
        double GetStallPercentage() const {
            return (double)stall_cycles / total_cycles * 100.0;
        }
    };
    
    // 解析SASS性能数据
    std::map<uint64_t, InstructionProfile> AnalyzeSASSPerformance(
        const uint8_t* counter_data,
        size_t data_size) 
    {
        std::map<uint64_t, InstructionProfile> profiles;
        
        // 解析counter数据
        NVPW_MetricsEvaluator_SetCounterData_Params setDataParams = {
            NVPW_MetricsEvaluator_SetCounterData_Params_STRUCT_SIZE
        };
        setDataParams.pMetricsEvaluator = m_metricsEvaluator;
        setDataParams.pCounterDataImage = counter_data;
        setDataParams.counterDataImageSize = data_size;
        NVPW_MetricsEvaluator_SetCounterData(&setDataParams);
        
        // 获取每条指令的性能数据
        for (const auto& range : m_ranges) {
            InstructionProfile profile = {};
            
            // 获取指令执行次数
            profile.execution_count = EvaluateMetric(
                "smsp__inst_executed.sum", range.id);
            
            // 计算周期数（基于IPC）
            double ipc = EvaluateMetric(
                "smsp__inst_executed.sum.per_cycle", range.id);
            profile.total_cycles = profile.execution_count / ipc;
            
            // 分析停顿原因
            AnalyzeStalls(range.id, profile.stalls);
            profile.stall_cycles = CalculateTotalStalls(profile.stalls);
            
            profiles[range.pc] = profile;
        }
        
        return profiles;
    }
    
private:
    void AnalyzeStalls(uint32_t range_id, 
                      InstructionProfile::StallBreakdown& stalls) 
    {
        // 长记分板停顿（通常是内存操作）
        double long_sb_stall = EvaluateMetric(
            "smsp__warp_issue_stalled_long_scoreboard_not_ready.avg.pct_of_peak_sustained_active",
            range_id);
        
        // 短记分板停顿（通常是ALU依赖）
        double short_sb_stall = EvaluateMetric(
            "smsp__warp_issue_stalled_short_scoreboard_not_ready.avg.pct_of_peak_sustained_active",
            range_id);
        
        // Barrier停顿
        double barrier_stall = EvaluateMetric(
            "smsp__warp_issue_stalled_barrier_not_reached.avg.pct_of_peak_sustained_active",
            range_id);
        
        // 转换为周期数
        uint64_t total_issue_cycles = EvaluateMetric(
            "smsp__cycles_active.sum", range_id);
        
        stalls.memory_dependency = total_issue_cycles * long_sb_stall / 100.0;
        stalls.execution_dependency = total_issue_cycles * short_sb_stall / 100.0;
        stalls.sync_dependency = total_issue_cycles * barrier_stall / 100.0;
    }
};
```

### 8.3 周期分析的关键概念

#### 8.3.1 指令延迟 vs 吞吐率
```cpp
// GPU指令的两个关键时序参数
struct InstructionLatency {
    // 延迟：从发射到结果可用的周期数
    uint32_t latency_cycles;
    
    // 吞吐率：连续发射相同指令的最小间隔
    uint32_t throughput_cycles;
    
    // 示例：NVIDIA GPU
    static InstructionLatency GetLatency(const std::string& opcode) {
        static std::map<std::string, InstructionLatency> latency_table = {
            // 算术指令
            {"FADD", {4, 1}},      // 浮点加法：4周期延迟，1周期吞吐
            {"FMUL", {4, 1}},      // 浮点乘法
            {"FFMA", {4, 1}},      // FMA
            {"MUFU", {4, 2}},      // 特殊函数单元
            
            // 内存指令
            {"LDG", {200, 4}},     // 全局内存加载：~200周期延迟
            {"LDS", {20, 2}},      // 共享内存加载：~20周期
            {"STG", {200, 4}},     // 全局内存存储
            
            // 控制流
            {"BRA", {4, 1}},       // 分支
            {"EXIT", {4, 1}},      // 退出
        };
        
        auto it = latency_table.find(opcode);
        return it != latency_table.end() ? it->second : InstructionLatency{4, 1};
    }
};
```

#### 8.3.2 Warp/Wavefront调度分析
```cpp
// GPU的并行执行模型
class WarpSchedulerAnalysis {
public:
    // NVIDIA: 每个SM有4个warp调度器，32线程/warp
    // AMD: 每个CU有4个SIMD单元，32或64线程/wavefront
    
    struct SchedulerMetrics {
        uint32_t active_warps;        // 活跃warp数
        uint32_t eligible_warps;      // 可调度warp数
        uint32_t stalled_warps;       // 停顿warp数
        float occupancy;              // 占用率
        
        float GetSchedulerEfficiency() const {
            return (float)eligible_warps / active_warps;
        }
    };
    
    // 分析调度效率
    SchedulerMetrics AnalyzeScheduling(const PerformanceData& data) {
        SchedulerMetrics metrics = {};
        
        // NVIDIA示例
        metrics.active_warps = data.GetMetric("smsp__warps_active.avg");
        metrics.eligible_warps = data.GetMetric("smsp__warps_eligible.avg");
        
        // 计算各种停顿
        uint32_t total_warps = data.GetMetric("smsp__warps_launched.sum");
        metrics.stalled_warps = metrics.active_warps - metrics.eligible_warps;
        
        // 理论最大warp数
        uint32_t max_warps = GetMaxWarpsPerSM();
        metrics.occupancy = (float)metrics.active_warps / max_warps;
        
        return metrics;
    }
    
    // 隐藏延迟所需的最小warp数
    uint32_t CalculateMinWarpsForLatencyHiding(
        uint32_t instruction_latency,
        uint32_t instructions_between_dependent) 
    {
        // Little's Law: 
        // Required Warps = Latency / Throughput
        return (instruction_latency + instructions_between_dependent - 1) 
               / instructions_between_dependent;
    }
};
```

### 8.4 实际案例：内存延迟分析

#### 8.4.1 识别内存瓶颈
```cpp
// 使用Instruction Trace识别和优化内存瓶颈
class MemoryBottleneckAnalyzer {
public:
    struct MemoryAccessPattern {
        uint64_t pc;                  // 指令地址
        uint32_t access_count;         // 访问次数
        uint32_t avg_latency_cycles;  // 平均延迟
        float coalescing_efficiency;  // 合并效率
        uint32_t cache_hit_rate;       // 缓存命中率
        
        bool IsBottleneck() const {
            return avg_latency_cycles > 100 && 
                   coalescing_efficiency < 0.5f;
        }
    };
    
    std::vector<MemoryAccessPattern> FindMemoryBottlenecks(
        const InstructionTraceData& trace_data) 
    {
        std::vector<MemoryAccessPattern> bottlenecks;
        
        // 分析每条内存指令
        for (const auto& inst : trace_data.instructions) {
            if (!IsMemoryInstruction(inst.opcode)) continue;
            
            MemoryAccessPattern pattern = {};
            pattern.pc = inst.pc;
            pattern.access_count = inst.execution_count;
            
            // 计算平均延迟
            pattern.avg_latency_cycles = CalculateAverageLatency(inst);
            
            // 分析访问模式
            pattern.coalescing_efficiency = AnalyzeCoalescing(inst);
            pattern.cache_hit_rate = GetCacheHitRate(inst);
            
            if (pattern.IsBottleneck()) {
                bottlenecks.push_back(pattern);
            }
        }
        
        // 按影响程度排序
        std::sort(bottlenecks.begin(), bottlenecks.end(),
            [](const auto& a, const auto& b) {
                uint64_t impact_a = a.access_count * a.avg_latency_cycles;
                uint64_t impact_b = b.access_count * b.avg_latency_cycles;
                return impact_a > impact_b;
            });
        
        return bottlenecks;
    }
    
    // 生成优化建议
    void GenerateOptimizationHints(const MemoryAccessPattern& pattern) {
        printf("Memory Bottleneck at PC 0x%llx:\n", pattern.pc);
        printf("  Average Latency: %u cycles\n", pattern.avg_latency_cycles);
        printf("  Coalescing Efficiency: %.2f%%\n", 
               pattern.coalescing_efficiency * 100);
        
        if (pattern.coalescing_efficiency < 0.5f) {
            printf("  Optimization: Improve memory access pattern for coalescing\n");
            printf("    - Ensure consecutive threads access consecutive addresses\n");
            printf("    - Consider using shared memory for irregular access\n");
        }
        
        if (pattern.cache_hit_rate < 50) {
            printf("  Optimization: Improve cache utilization\n");
            printf("    - Increase data locality\n");
            printf("    - Consider tiling/blocking algorithms\n");
        }
        
        if (pattern.avg_latency_cycles > 200) {
            printf("  Optimization: Hide memory latency\n");
            printf("    - Increase occupancy (more warps)\n");
            printf("    - Add computation between memory operations\n");
            printf("    - Consider prefetching\n");
        }
    }
};
```

### 8.5 综合分析工具

#### 8.5.1 统一的周期分析接口
```cpp
// 跨平台的指令周期分析器
class UnifiedInstructionAnalyzer {
public:
    // 统一的分析结果
    struct InstructionMetrics {
        uint64_t pc;
        std::string disassembly;
        uint64_t execution_count;
        uint64_t total_cycles;
        uint64_t active_cycles;
        uint64_t stall_cycles;
        
        // 停顿分类（百分比）
        float memory_stall_pct;
        float execution_stall_pct;
        float synchronization_stall_pct;
        float scheduler_stall_pct;
        
        float GetEfficiency() const {
            return (float)active_cycles / total_cycles;
        }
        
        float GetIPC() const {
            return (float)execution_count / active_cycles;
        }
    };
    
    // 分析入口
    std::vector<InstructionMetrics> Analyze(
        CGPUPerfSessionId session,
        CGPUBackend backend) 
    {
        std::vector<InstructionMetrics> results;
        
        switch(backend) {
        case CGPU_BACKEND_D3D12:
        case CGPU_BACKEND_VULKAN:
            if (IsNVIDIA()) {
                results = AnalyzeNVIDIA_SASS(session);
            } else if (IsAMD()) {
                results = AnalyzeAMD_SQTT(session);
            }
            break;
        }
        
        // 计算关键统计
        ComputeStatistics(results);
        
        return results;
    }
    
private:
    void ComputeStatistics(std::vector<InstructionMetrics>& metrics) {
        // 计算总体统计
        uint64_t total_cycles = 0;
        uint64_t total_stalls = 0;
        
        for (auto& m : metrics) {
            total_cycles += m.total_cycles;
            total_stalls += m.stall_cycles;
        }
        
        printf("\n=== Instruction Cycle Analysis Summary ===\n");
        printf("Total Cycles: %llu\n", total_cycles);
        printf("Stall Cycles: %llu (%.2f%%)\n", 
               total_stalls, (float)total_stalls/total_cycles * 100);
        
        // 找出最耗时的指令
        std::sort(metrics.begin(), metrics.end(),
            [](const auto& a, const auto& b) {
                return a.total_cycles > b.total_cycles;
            });
        
        printf("\nTop 5 Most Expensive Instructions:\n");
        for (int i = 0; i < std::min(5, (int)metrics.size()); i++) {
            const auto& m = metrics[i];
            printf("  [%d] PC=0x%llx: %llu cycles (%.2f%% stalled)\n",
                   i+1, m.pc, m.total_cycles, 
                   (float)m.stall_cycles/m.total_cycles * 100);
        }
    }
};
```

## 9. 已知限制

1. **NVIDIA限制**:
   - 某些GPU不支持GPU Periodic Sampler
   - SLI配置不支持
   - 加密挖矿GPU (CMP) 不支持
   - SASS追踪需要较新的驱动版本

2. **AMD限制**:
   - 旧GPU可能不支持SPM
   - 某些counter组合可能增加pass数量
   - SQTT缓冲区大小受VRAM限制
   - GCN和RDNA架构的SQTT格式不同

3. **统一API限制**:
   - Instruction Trace实现差异较大，需要vendor特定接口
   - 实时采样精度依赖硬件支持
   - 指令格式无法统一（SASS vs GCN/RDNA ISA）
   - 数据格式和解析方式完全不同
