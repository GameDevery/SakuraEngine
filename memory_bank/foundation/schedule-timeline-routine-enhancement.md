# ScheduleTimeline例程切分增强方案

## 增强思路

在现有的`ScheduleTimeline`阶段中，我们已经生成了：
1. `QueueScheduleInfo` - 每个队列的Pass序列
2. `SyncRequirement` - 队列间的同步需求

现在只需要在这个基础上添加**例程切分逻辑**，将每个队列的Pass序列按SyncPoint切分为小的执行例程。

## 数据结构增强

### 1. 在现有结构中添加例程相关类型

```cpp
// 在 schedule_timeline.hpp 中添加

// 执行例程：队列Timeline中被SyncPoint切分的片段
struct ExecutionRoutine {
    uint32_t routine_id;                 // 例程ID
    uint32_t queue_index;                // 所属队列
    skr::Vector<PassNode*> passes;       // 例程内的Pass序列
    
    // 例程边界
    PassNode* first_pass = nullptr;      // 首个Pass
    PassNode* last_pass = nullptr;       // 最后一个Pass
    SyncRequirement* terminal_sync = nullptr;  // 结束时的同步点(可选)
    
    // 例程的资源需求(可选，用于优化)
    skr::Vector<ResourceNode*> input_resources;   // 例程开始时需要的资源
    skr::Vector<ResourceNode*> output_resources;  // 例程结束时产生的资源
    skr::Vector<ResourceNode*> temp_resources;    // 例程内部临时资源
};

// 切分后的队列调度信息
struct SegmentedQueueSchedule {
    ERenderGraphQueueType queue_type;
    uint32_t queue_index;
    skr::Vector<ExecutionRoutine> routines;  // 按顺序排列的例程
    
    // 统计信息
    uint32_t total_passes = 0;
    uint32_t total_routines = 0;
};

// 增强的Timeline调度结果
struct TimelineScheduleResult {
    // 原有字段保持不变
    skr::Vector<QueueScheduleInfo> queue_schedules;
    skr::Vector<SyncRequirement> sync_requirements;
    skr::FlatHashMap<PassNode*, uint32_t> pass_queue_assignments;
    
    // 新增：例程切分结果
    skr::Vector<SegmentedQueueSchedule> segmented_queues;  // 切分后的队列
    skr::FlatHashMap<PassNode*, uint32_t> pass_routine_assignments;  // Pass到例程的映射
    
    // 统计信息
    uint32_t total_routines = 0;
    uint32_t max_routine_size = 0;        // 最大例程的Pass数量
    float avg_routine_size = 0.0f;        // 平均例程大小
};
```

### 2. 增强ScheduleTimeline类接口

```cpp
class SKR_RENDER_GRAPH_API ScheduleTimeline : public IRenderGraphPhase {
public:
    // 现有接口保持不变...
    
    // 新增：例程切分配置
    struct RoutineSegmentationConfig {
        bool enable_routine_segmentation = true;    // 启用例程切分
        bool optimize_routine_resources = true;     // 优化例程资源分析
        uint32_t min_routine_size = 1;              // 最小例程大小
        uint32_t max_routine_size = 32;             // 最大例程大小
    };
    
    // 构造函数增强
    ScheduleTimeline(const PassDependencyAnalysis& dependency_analysis, 
                    const ScheduleTimelineConfig& config = {},
                    const RoutineSegmentationConfig& routine_config = {});
    
    // 新增：获取切分后的结果
    const skr::Vector<SegmentedQueueSchedule>& get_segmented_queues() const {
        return schedule_result.segmented_queues;
    }
    
    // 新增：查询Pass所属的例程
    uint32_t get_pass_routine_id(PassNode* pass) const;
    const ExecutionRoutine* get_routine_by_id(uint32_t routine_id) const;
    
    // 新增：调试接口
    void dump_routine_segmentation() const;

private:
    // 新增：例程切分逻辑
    void segment_queues_into_routines() SKR_NOEXCEPT;
    void segment_single_queue(const QueueScheduleInfo& queue_schedule, 
                             SegmentedQueueSchedule& segmented_queue) SKR_NOEXCEPT;
    
    // 新增：例程资源分析(可选优化)
    void analyze_routine_resources(ExecutionRoutine& routine) SKR_NOEXCEPT;
    void optimize_routine_boundaries() SKR_NOEXCEPT;
    
    // 新增：例程验证
    void validate_routine_segmentation() const SKR_NOEXCEPT;

private:
    // 新增配置
    RoutineSegmentationConfig routine_config_;
    
    // 现有成员保持不变...
};
```

## 核心算法实现

### 1. 例程切分主逻辑

```cpp
void ScheduleTimeline::segment_queues_into_routines() SKR_NOEXCEPT {
    if (!routine_config_.enable_routine_segmentation) {
        SKR_LOG_DEBUG(u8"Routine segmentation disabled, skipping");
        return;
    }
    
    schedule_result.segmented_queues.clear();
    schedule_result.segmented_queues.reserve(schedule_result.queue_schedules.size());
    
    uint32_t global_routine_id = 0;
    
    // 为每个队列进行例程切分
    for (const auto& queue_schedule : schedule_result.queue_schedules) {
        SegmentedQueueSchedule segmented_queue;
        segmented_queue.queue_type = queue_schedule.queue_type;
        segmented_queue.queue_index = queue_schedule.queue_index;
        
        segment_single_queue(queue_schedule, segmented_queue);
        
        // 分配全局例程ID
        for (auto& routine : segmented_queue.routines) {
            routine.routine_id = global_routine_id++;
            
            // 建立Pass到例程的映射
            for (auto* pass : routine.passes) {
                schedule_result.pass_routine_assignments[pass] = routine.routine_id;
            }
        }
        
        schedule_result.segmented_queues.add(std::move(segmented_queue));
    }
    
    // 统计信息
    calculate_segmentation_statistics();
    
    SKR_LOG_INFO(u8"Queue segmentation complete: %u total routines across %zu queues",
        schedule_result.total_routines, schedule_result.segmented_queues.size());
}
```

### 2. 单队列切分逻辑

```cpp
void ScheduleTimeline::segment_single_queue(const QueueScheduleInfo& queue_schedule,
                                           SegmentedQueueSchedule& segmented_queue) SKR_NOEXCEPT {
    
    if (queue_schedule.scheduled_passes.empty()) {
        return;
    }
    
    // 收集该队列相关的同步点
    skr::Vector<SyncRequirement*> queue_sync_points;
    for (auto& sync_req : schedule_result.sync_requirements) {
        // 找到该队列作为信号源的同步点
        if (sync_req.signal_queue_index == queue_schedule.queue_index) {
            queue_sync_points.add(&sync_req);
        }
    }
    
    // 按Pass顺序排序同步点
    std::sort(queue_sync_points.begin(), queue_sync_points.end(),
        [&queue_schedule](const SyncRequirement* a, const SyncRequirement* b) {
            auto pos_a = std::find(queue_schedule.scheduled_passes.begin(),
                                  queue_schedule.scheduled_passes.end(),
                                  a->signal_after_pass);
            auto pos_b = std::find(queue_schedule.scheduled_passes.begin(),
                                  queue_schedule.scheduled_passes.end(),
                                  b->signal_after_pass);
            return pos_a < pos_b;
        });
    
    // 按同步点切分Pass序列
    uint32_t current_pass_index = 0;
    uint32_t sync_point_index = 0;
    
    while (current_pass_index < queue_schedule.scheduled_passes.size()) {
        ExecutionRoutine routine;
        routine.queue_index = queue_schedule.queue_index;
        
        // 确定例程的结束点
        uint32_t routine_end_index = queue_schedule.scheduled_passes.size();
        SyncRequirement* terminal_sync = nullptr;
        
        if (sync_point_index < queue_sync_points.size()) {
            auto* sync_req = queue_sync_points[sync_point_index];
            
            // 找到sync_req.signal_after_pass在Pass序列中的位置
            auto it = std::find(queue_schedule.scheduled_passes.begin(),
                               queue_schedule.scheduled_passes.end(),
                               sync_req->signal_after_pass);
            
            if (it != queue_schedule.scheduled_passes.end()) {
                routine_end_index = std::distance(queue_schedule.scheduled_passes.begin(), it) + 1;
                terminal_sync = sync_req;
                sync_point_index++;
            }
        }
        
        // 限制例程大小
        if (routine_end_index - current_pass_index > routine_config_.max_routine_size) {
            routine_end_index = current_pass_index + routine_config_.max_routine_size;
            terminal_sync = nullptr;  // 不是真正的同步边界
        }
        
        // 创建例程
        routine.passes.reserve(routine_end_index - current_pass_index);
        for (uint32_t i = current_pass_index; i < routine_end_index; ++i) {
            routine.passes.add(queue_schedule.scheduled_passes[i]);
        }
        
        routine.first_pass = routine.passes.front();
        routine.last_pass = routine.passes.back();
        routine.terminal_sync = terminal_sync;
        
        // 可选：分析例程资源需求
        if (routine_config_.optimize_routine_resources) {
            analyze_routine_resources(routine);
        }
        
        segmented_queue.routines.add(std::move(routine));
        current_pass_index = routine_end_index;
    }
    
    // 统计信息
    segmented_queue.total_passes = queue_schedule.scheduled_passes.size();
    segmented_queue.total_routines = segmented_queue.routines.size();
    
    SKR_LOG_DEBUG(u8"Queue %u segmented: %u passes -> %u routines",
        queue_schedule.queue_index,
        segmented_queue.total_passes,
        segmented_queue.total_routines);
}
```

### 3. 例程资源分析(可选优化)

```cpp
void ScheduleTimeline::analyze_routine_resources(ExecutionRoutine& routine) SKR_NOEXCEPT {
    // 这是一个可选的优化，用于预分析例程的资源需求
    // 有助于后续的资源管理和内存优化
    
    skr::Set<ResourceNode*> all_resources;
    skr::Set<ResourceNode*> input_resources;
    skr::Set<ResourceNode*> output_resources;
    
    for (auto* pass : routine.passes) {
        // 获取Pass的资源访问信息
        const auto* pass_info = dependency_analysis.get_pass_info(pass);
        if (!pass_info) continue;
        
        // 收集所有资源
        for (const auto& access : pass_info->resource_accesses) {
            all_resources.add(access.resource);
            
            // 分析输入/输出模式
            if (access.resource->is_imported() || 
                is_resource_produced_before_routine(access.resource, routine)) {
                input_resources.add(access.resource);
            }
            
            if (access.access_type & EResourceAccess::Write) {
                output_resources.add(access.resource);
            }
        }
    }
    
    // 转换为Vector存储
    routine.input_resources.assign(input_resources.begin(), input_resources.end());
    routine.output_resources.assign(output_resources.begin(), output_resources.end());
    
    // 计算临时资源(在例程内创建和销毁)
    for (auto* resource : all_resources) {
        if (!input_resources.contains(resource) && 
            !is_resource_used_after_routine(resource, routine)) {
            routine.temp_resources.add(resource);
        }
    }
    
    SKR_LOG_TRACE(u8"Routine %u resources: %zu input, %zu output, %zu temp",
        routine.routine_id,
        routine.input_resources.size(),
        routine.output_resources.size(),
        routine.temp_resources.size());
}
```

### 4. 调试和统计

```cpp
void ScheduleTimeline::dump_routine_segmentation() const {
    SKR_LOG_INFO(u8"========== Routine Segmentation Report ==========");
    
    for (const auto& segmented_queue : schedule_result.segmented_queues) {
        SKR_LOG_INFO(u8"Queue %u (%s): %u passes, %u routines",
            segmented_queue.queue_index,
            get_queue_type_name(segmented_queue.queue_type),
            segmented_queue.total_passes,
            segmented_queue.total_routines);
        
        for (const auto& routine : segmented_queue.routines) {
            SKR_LOG_INFO(u8"  Routine %u: %zu passes [%s -> %s]%s",
                routine.routine_id,
                routine.passes.size(),
                routine.first_pass->get_name(),
                routine.last_pass->get_name(),
                routine.terminal_sync ? " (sync)" : "");
            
            if (routine_config_.optimize_routine_resources) {
                SKR_LOG_INFO(u8"    Resources: %zu in, %zu out, %zu temp",
                    routine.input_resources.size(),
                    routine.output_resources.size(),
                    routine.temp_resources.size());
            }
        }
    }
    
    SKR_LOG_INFO(u8"Total: %u routines, avg size: %.1f passes",
        schedule_result.total_routines,
        schedule_result.avg_routine_size);
    SKR_LOG_INFO(u8"===============================================");
}

void ScheduleTimeline::calculate_segmentation_statistics() {
    uint32_t total_routines = 0;
    uint32_t total_passes = 0;
    uint32_t max_routine_size = 0;
    
    for (const auto& segmented_queue : schedule_result.segmented_queues) {
        total_routines += segmented_queue.total_routines;
        total_passes += segmented_queue.total_passes;
        
        for (const auto& routine : segmented_queue.routines) {
            max_routine_size = std::max(max_routine_size, 
                                       static_cast<uint32_t>(routine.passes.size()));
        }
    }
    
    schedule_result.total_routines = total_routines;
    schedule_result.max_routine_size = max_routine_size;
    schedule_result.avg_routine_size = total_routines > 0 ? 
        static_cast<float>(total_passes) / total_routines : 0.0f;
}
```

## 集成到现有执行流程

### 在on_execute中添加切分逻辑

```cpp
void ScheduleTimeline::on_execute(RenderGraph* graph, RenderGraphProfiler* profiler) SKR_NOEXCEPT {
    SKR_LOG_DEBUG(u8"ScheduleTimeline: Starting timeline scheduling and fence allocation");
    
    // 现有逻辑保持不变...
    clear_frame_data();
    analyze_dependencies(graph);
    assign_passes_to_queues(graph);
    calculate_sync_requirements(graph);
    
    // 新增：例程切分逻辑
    segment_queues_into_routines();
    
    // 可选：输出调试信息
    if (config.enable_debug_output) {
        dump_timeline_result(u8"Timeline Schedule", schedule_result);
        if (routine_config_.enable_routine_segmentation) {
            dump_routine_segmentation();
        }
    }
    
    SKR_LOG_DEBUG(u8"ScheduleTimeline: Completed with %zu queues, %u routines",
        schedule_result.queue_schedules.size(),
        schedule_result.total_routines);
}
```

## 优势分析

### 1. 架构一致性 ✅
- 不破坏现有Phase结构
- 逻辑内聚在ScheduleTimeline中
- 对外接口保持兼容

### 2. 自然的切分边界 ✅
- SyncPoint天然就是执行边界
- 切分逻辑简单清晰
- 易于理解和维护

### 3. 渐进式增强 ✅
- 可以通过配置开关启用/禁用
- 不影响现有功能
- 便于测试和验证

### 4. 为后续优化铺路 ✅
- 为精确资源管理提供基础
- 支持例程级别的性能分析
- 便于实现内存别名优化

这个设计真的很优雅！它在保持现有架构稳定性的同时，为更精确的资源管理奠定了基础。