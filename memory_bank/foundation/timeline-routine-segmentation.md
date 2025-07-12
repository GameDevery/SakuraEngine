# Timeline例程切分方案设计

## 核心思路

将每个队列的Timeline按SyncPoint切分为多个小的**执行例程(Execution Routines)**，每个例程：
1. 独立管理自己的资源生命周期
2. 在例程边界进行资源的Acquire/Release
3. 通过迭代例程实现精确的资源管理

## 概念对比

### 传统方式：巨大Timeline
```cpp
// 问题：资源必须在整个Timeline期间保持活跃
Timeline Graphics_Queue {
    passes: [P1, P2, P3, ..., P50],
    resources: {
        Buffer A: [P1_start → P50_end],  // 生命周期过长
        Buffer B: [P5_start → P45_end],  // 无法精确释放
        Buffer C: [P10_start → P30_end]
    }
}
```

### 新方式：例程切分
```cpp
// 解决：每个例程独立管理资源
Timeline Graphics_Queue {
    Routine_1: {
        passes: [P1, P2, P3],
        sync_end: SyncPoint_1,
        resources: {
            Buffer A: acquire_at_start → release_at_end,
            Buffer B: acquire_when_needed → release_immediately
        }
    },
    
    Routine_2: {
        passes: [P4, P5, P6, P7], 
        sync_end: SyncPoint_2,
        resources: {
            Buffer C: acquire_at_start → release_at_end,
            Buffer D: acquire_when_needed → release_immediately  
        }
    },
    
    // Buffer A和Buffer C可以别名！因为它们在不同例程中
}
```

## 具体设计

### 1. Timeline例程数据结构

```cpp
namespace skr::render_graph {

// 执行例程：Timeline的一个片段
struct ExecutionRoutine {
    uint32_t queue_id;
    uint32_t routine_index;
    
    // 例程内的Pass序列
    skr::Vector<PassNode*> passes;
    
    // 例程的边界
    PassNode* start_pass;
    PassNode* end_pass;
    SyncPoint* terminal_sync_point;  // 例程结束时的同步点
    
    // 例程内的资源需求
    skr::Vector<ResourceNode*> required_resources;
    skr::Vector<ResourceNode*> produced_resources;
    
    // 资源生命周期（仅在例程内部）
    skr::Map<ResourceNode*, RoutineResourceLifetime> resource_lifetimes;
};

// 例程内的资源生命周期
struct RoutineResourceLifetime {
    PassNode* first_use;     // 例程内首次使用
    PassNode* last_use;      // 例程内最后使用
    EResourceAccess access_pattern;
    
    // 是否可以在例程结束时释放
    bool can_release_at_end = true;
    
    // 是否需要在例程开始时获取
    bool need_acquire_at_start = true;
};

// 切分后的Timeline
struct SegmentedTimeline {
    uint32_t queue_id;
    ERenderGraphQueueType queue_type;
    
    // 按顺序排列的例程
    skr::Vector<ExecutionRoutine> routines;
    
    // 例程间的同步关系
    skr::Vector<SyncPoint> inter_routine_sync_points;
};

} // namespace
```

### 2. Timeline切分算法

```cpp
class TimelineSegmentationPhase : public IRenderGraphPhase {
public:
    void on_execute(RenderGraph* graph, RenderGraphProfiler* profiler) override {
        const auto& timeline_result = timeline_schedule_.get_schedule_result();
        
        segmented_timelines_.clear();
        segmented_timelines_.reserve(timeline_result.queue_timelines.size());
        
        // 为每个队列创建切分的Timeline
        for (const auto& queue_timeline : timeline_result.queue_timelines) {
            auto segmented = segment_queue_timeline(queue_timeline);
            segmented_timelines_.add(std::move(segmented));
        }
    }
    
private:
    SegmentedTimeline segment_queue_timeline(const QueueTimeline& queue_timeline) {
        SegmentedTimeline result;
        result.queue_id = queue_timeline.queue_id;
        result.queue_type = queue_timeline.queue_type;
        
        // 获取该队列的所有同步点
        auto sync_points = get_sync_points_for_queue(queue_timeline.queue_id);
        
        // 按同步点切分Pass序列
        uint32_t routine_index = 0;
        uint32_t pass_start = 0;
        
        for (auto* sync_point : sync_points) {
            // 找到同步点对应的Pass位置
            uint32_t pass_end = find_pass_index_before_sync(sync_point);
            
            // 创建例程
            ExecutionRoutine routine = create_routine(
                queue_timeline, routine_index++, pass_start, pass_end, sync_point);
            
            result.routines.add(std::move(routine));
            pass_start = pass_end + 1;
        }
        
        // 处理最后一个例程（如果有剩余Pass）
        if (pass_start < queue_timeline.passes.size()) {
            ExecutionRoutine final_routine = create_final_routine(
                queue_timeline, routine_index, pass_start);
            result.routines.add(std::move(final_routine));
        }
        
        return result;
    }
    
    ExecutionRoutine create_routine(const QueueTimeline& queue_timeline,
                                   uint32_t routine_index,
                                   uint32_t pass_start, uint32_t pass_end,
                                   SyncPoint* sync_point) {
        ExecutionRoutine routine;
        routine.queue_id = queue_timeline.queue_id;
        routine.routine_index = routine_index;
        routine.terminal_sync_point = sync_point;
        
        // 复制Pass序列
        for (uint32_t i = pass_start; i <= pass_end; ++i) {
            routine.passes.add(queue_timeline.passes[i]);
        }
        
        routine.start_pass = routine.passes.front();
        routine.end_pass = routine.passes.back();
        
        // 分析例程内的资源需求
        analyze_routine_resource_requirements(routine);
        
        return routine;
    }
    
    void analyze_routine_resource_requirements(ExecutionRoutine& routine) {
        // 收集例程内所有使用的资源
        skr::Set<ResourceNode*> all_resources;
        
        for (auto* pass : routine.passes) {
            const auto* pass_info = pass_info_analysis_.get_pass_info(pass);
            if (!pass_info) continue;
            
            for (const auto& access : pass_info->resource_info.all_resource_accesses) {
                all_resources.add(access.resource);
            }
        }
        
        // 为每个资源计算例程内的生命周期
        for (auto* resource : all_resources) {
            auto lifetime = compute_routine_resource_lifetime(routine, resource);
            routine.resource_lifetimes[resource] = lifetime;
            
            routine.required_resources.add(resource);
        }
    }
    
    RoutineResourceLifetime compute_routine_resource_lifetime(
        const ExecutionRoutine& routine, ResourceNode* resource) {
        
        RoutineResourceLifetime lifetime;
        lifetime.first_use = nullptr;
        lifetime.last_use = nullptr;
        
        // 在例程内查找首次和最后使用
        for (auto* pass : routine.passes) {
            if (pass_accesses_resource(pass, resource)) {
                if (!lifetime.first_use) {
                    lifetime.first_use = pass;
                }
                lifetime.last_use = pass;
                
                // 分析访问模式
                auto access = get_resource_access_type(pass, resource);
                lifetime.access_pattern |= access;
            }
        }
        
        // 判断是否可以在例程结束时释放
        lifetime.can_release_at_end = !is_resource_used_in_later_routines(resource, routine);
        
        // 判断是否需要在例程开始时获取
        lifetime.need_acquire_at_start = !is_resource_produced_in_routine(resource, routine);
        
        return lifetime;
    }

private:
    // 依赖的Phase
    const ScheduleTimeline& timeline_schedule_;
    const PassInfoAnalysis& pass_info_analysis_;
    
    // 输出结果
    skr::Vector<SegmentedTimeline> segmented_timelines_;
};
```

### 3. 运行时例程执行器

```cpp
class RoutineExecutor {
public:
    struct ExecutionContext {
        DynamicResourcePool* resource_pool;
        skr::Map<ResourceNode*, ResourceHandle> acquired_resources;
        uint32_t current_routine_index = 0;
    };
    
    void execute_segmented_timeline(const SegmentedTimeline& timeline, 
                                   ExecutionContext& context) {
        
        for (const auto& routine : timeline.routines) {
            execute_routine(routine, context);
        }
    }
    
private:
    void execute_routine(const ExecutionRoutine& routine, ExecutionContext& context) {
        // 第一步：获取例程需要的资源
        acquire_routine_resources(routine, context);
        
        // 第二步：执行例程内的所有Pass
        execute_routine_passes(routine, context);
        
        // 第三步：释放可以释放的资源
        release_routine_resources(routine, context);
        
        // 第四步：处理同步点
        handle_routine_sync_point(routine, context);
    }
    
    void acquire_routine_resources(const ExecutionRoutine& routine, 
                                  ExecutionContext& context) {
        
        skr::Vector<ResourceDesc> resources_to_acquire;
        
        // 收集需要在例程开始时获取的资源
        for (const auto& [resource, lifetime] : routine.resource_lifetimes) {
            if (lifetime.need_acquire_at_start && 
                !context.acquired_resources.contains(resource)) {
                
                ResourceDesc desc = create_resource_desc(resource);
                resources_to_acquire.add(desc);
            }
        }
        
        // 批量获取资源
        skr::Vector<ResourceHandle> acquired_handles;
        context.resource_pool->batch_acquire(resources_to_acquire, acquired_handles);
        
        // 更新上下文
        for (size_t i = 0; i < resources_to_acquire.size(); ++i) {
            auto* resource = find_resource_by_desc(resources_to_acquire[i]);
            context.acquired_resources[resource] = acquired_handles[i];
        }
    }
    
    void execute_routine_passes(const ExecutionRoutine& routine, 
                               ExecutionContext& context) {
        
        for (auto* pass : routine.passes) {
            // 绑定Pass使用的资源句柄
            bind_pass_resources(pass, context.acquired_resources);
            
            // 执行Pass
            execute_pass(pass);
        }
    }
    
    void release_routine_resources(const ExecutionRoutine& routine, 
                                  ExecutionContext& context) {
        
        skr::Vector<ResourceHandle> resources_to_release;
        
        // 收集可以在例程结束时释放的资源
        for (const auto& [resource, lifetime] : routine.resource_lifetimes) {
            if (lifetime.can_release_at_end) {
                auto it = context.acquired_resources.find(resource);
                if (it) {
                    resources_to_release.add(it.value());
                    context.acquired_resources.erase(it);
                }
            }
        }
        
        // 批量释放资源
        context.resource_pool->batch_release(resources_to_release);
    }
    
    void handle_routine_sync_point(const ExecutionRoutine& routine, 
                                  ExecutionContext& context) {
        
        if (routine.terminal_sync_point) {
            // 处理同步点逻辑
            // 这里可以是GPU fence，也可以是其他同步机制
            handle_sync_point_impl(routine.terminal_sync_point);
        }
        
        context.current_routine_index++;
    }
};
```

### 4. 资源别名优化增强

```cpp
class RoutineBasedAliasingOptimizer {
public:
    // 基于例程的别名分析
    bool can_resources_alias_across_routines(ResourceNode* res_a, ResourceNode* res_b,
                                            const SegmentedTimeline& timeline) {
        
        // 找到两个资源分别在哪些例程中使用
        auto routines_a = find_routines_using_resource(res_a, timeline);
        auto routines_b = find_routines_using_resource(res_b, timeline);
        
        // 检查例程是否有重叠
        for (auto* routine_a : routines_a) {
            for (auto* routine_b : routines_b) {
                if (routines_overlap(routine_a, routine_b)) {
                    return false;  // 有重叠，不能别名
                }
            }
        }
        
        return true;  // 无重叠，可以别名
    }
    
    // 计算内存节省效果
    float calculate_memory_savings(const skr::Vector<SegmentedTimeline>& timelines) {
        size_t total_memory_without_aliasing = 0;
        size_t total_memory_with_aliasing = 0;
        
        // 计算无别名时的内存需求
        for (const auto& timeline : timelines) {
            for (const auto& routine : timeline.routines) {
                for (const auto& [resource, lifetime] : routine.resource_lifetimes) {
                    total_memory_without_aliasing += get_resource_size(resource);
                }
            }
        }
        
        // 计算有别名时的内存需求
        auto aliasing_groups = compute_optimal_aliasing_groups(timelines);
        for (const auto& group : aliasing_groups) {
            // 每个别名组只需要最大资源的内存
            size_t max_size = 0;
            for (auto* resource : group.resources) {
                max_size = std::max(max_size, get_resource_size(resource));
            }
            total_memory_with_aliasing += max_size;
        }
        
        return 1.0f - (float)total_memory_with_aliasing / total_memory_without_aliasing;
    }
    
private:
    bool routines_overlap(const ExecutionRoutine* routine_a, 
                         const ExecutionRoutine* routine_b) {
        
        // 同一队列的例程按顺序执行，不重叠
        if (routine_a->queue_id == routine_b->queue_id) {
            return false;
        }
        
        // 不同队列的例程需要检查同步关系
        return check_cross_queue_routine_overlap(routine_a, routine_b);
    }
};
```

## 优势分析

### 1. 精确的资源生命周期管理 ⭐⭐⭐⭐⭐
```cpp
// 传统方法：保守估计
Resource buffer = allocate_for_entire_timeline();  // 可能浪费50%+

// 例程方法：精确管理
for (auto& routine : timeline.routines) {
    auto resources = acquire_for_routine(routine);   // 精确分配
    execute_routine(routine, resources);
    release_routine_resources(routine, resources);   // 立即释放
}
```

### 2. 更好的别名机会 ⭐⭐⭐⭐⭐
```cpp
// 例程A和例程C之间可以共享资源（即使在同一队列）
Routine A: Buffer X [100MB] → SyncPoint → 释放
Routine B: 其他资源
Routine C: Buffer Y [100MB] → 可以重用Buffer X的内存！
```

### 3. 简化的调试 ⭐⭐⭐⭐
```cpp
// 清晰的执行边界
void debug_routine_execution() {
    for (auto& routine : timeline.routines) {
        log("例程 %d 开始", routine.routine_index);
        log("  获取资源: %s", list_acquired_resources(routine));
        execute_routine(routine);
        log("  释放资源: %s", list_released_resources(routine));
        log("例程 %d 结束", routine.routine_index);
    }
}
```

## 与现有Phase结构的完美集成

```cpp
// 现有Phase保持不变，只是在Timeline阶段后增加切分
PassInfoAnalysis → PassDependencyAnalysis → ScheduleTimeline 
                                                ↓
                                        TimelineSegmentation  // 新增
                                                ↓
                                        RoutineExecution      // 新增
```

这个方案真的很聪明！它保持了现有架构的稳定性，同时通过**例程切分**实现了更精确的资源管理。这比我之前理解的协程方案要优雅得多。

你觉得从哪个部分开始实现比较好？我建议先实现Timeline切分算法，然后再逐步添加例程执行器。