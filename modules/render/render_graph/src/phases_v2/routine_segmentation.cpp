#include "SkrRenderGraph/phases_v2/routine_segmentation.hpp"
#include "SkrRenderGraph/phases_v2/schedule_timeline.hpp"
#include "SkrRenderGraph/phases_v2/schedule_reorder.hpp"
#include "SkrRenderGraph/frontend/pass_node.hpp"
#include "SkrRenderGraph/frontend/resource_node.hpp"
#include <algorithm>

namespace skr {
namespace render_graph {

RoutineSegmentationPhase::RoutineSegmentationPhase(const ScheduleTimeline& timeline_phase, 
                                                   const ExecutionReorderPhase& reorder_phase,
                                                   const RoutineSegmentationConfig& config)
    : config(config), timeline_phase(timeline_phase), reorder_phase(reorder_phase)
{
}

RoutineSegmentationPhase::~RoutineSegmentationPhase() = default;

void RoutineSegmentationPhase::on_initialize(RenderGraph* graph) SKR_NOEXCEPT
{
    // 清理结果
    clear_frame_data();
}

void RoutineSegmentationPhase::on_finalize(RenderGraph* graph) SKR_NOEXCEPT
{
    // 清理所有状态
    clear_frame_data();
}

void RoutineSegmentationPhase::clear_frame_data() SKR_NOEXCEPT
{
    result.segmented_queues.clear();
    result.pass_routine_assignments.clear();
    result.total_routines = 0;
    result.max_routine_size = 0;
    result.avg_routine_size = 0.0f;
}

void RoutineSegmentationPhase::on_execute(RenderGraph* graph, RenderGraphProfiler* profiler) SKR_NOEXCEPT
{
    SKR_LOG_DEBUG(u8"RoutineSegmentationPhase: Starting routine segmentation");
    
    // 清空上一帧的数据
    clear_frame_data();
    
    // 执行例程切分
    segment_queues_into_routines();
    
    // 可选：输出调试信息
    if (config.enable_debug_output) {
        dump_routine_segmentation();
    }
    
    SKR_LOG_DEBUG(u8"RoutineSegmentationPhase: Completed with %u total routines",
                  result.total_routines);
}

void RoutineSegmentationPhase::segment_queues_into_routines() SKR_NOEXCEPT 
{
    if (!config.enable_routine_segmentation) {
        SKR_LOG_DEBUG(u8"Routine segmentation disabled, skipping");
        return;
    }
    
    // 获取Timeline的调度结果
    const auto& timelines = reorder_phase.get_optimized_timeline();
    
    result.segmented_queues.clear();
    result.segmented_queues.reserve(timelines.size());
    
    uint32_t global_routine_id = 0;
    
    // 为每个队列进行例程切分
    for (const auto& queue_schedule : timelines) {
        SegmentedQueueSchedule segmented_queue;
        segmented_queue.queue_type = queue_schedule.queue_type;
        segmented_queue.queue_index = queue_schedule.queue_index;
        
        segment_single_queue(queue_schedule, segmented_queue);
        
        // 分配全局例程ID
        for (auto& routine : segmented_queue.routines) {
            routine.routine_id = global_routine_id++;
            
            // 建立Pass到例程的映射
            for (auto* pass : routine.passes) {
                result.pass_routine_assignments[pass] = routine.routine_id;
            }
        }
        
        result.segmented_queues.add(std::move(segmented_queue));
    }
    
    // 统计信息
    calculate_segmentation_statistics();
    
    SKR_LOG_INFO(u8"Queue segmentation complete: %u total routines across %zu queues",
        result.total_routines, result.segmented_queues.size());
}

void RoutineSegmentationPhase::segment_single_queue(const QueueScheduleInfo& queue_schedule,
                                                   SegmentedQueueSchedule& segmented_queue) SKR_NOEXCEPT 
{
    if (queue_schedule.scheduled_passes.is_empty()) {
        return;
    }
    
    // 获取Timeline的同步需求
    const auto& sync_requirements = timeline_phase.get_schedule_result().sync_requirements;
    
    // 收集该队列相关的同步点
    skr::Vector<SyncRequirement*> queue_sync_points;
    for (auto& sync_req : sync_requirements) {
        // 找到该队列作为信号源的同步点
        if (sync_req.signal_queue_index == queue_schedule.queue_index) {
            queue_sync_points.add(const_cast<SyncRequirement*>(&sync_req));
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
        if (routine_end_index - current_pass_index > config.max_routine_size) {
            routine_end_index = current_pass_index + config.max_routine_size;
            terminal_sync = nullptr;  // 不是真正的同步边界
        }
        
        // 确保最小例程大小
        if (routine_end_index - current_pass_index < config.min_routine_size) {
            routine_end_index = std::min(current_pass_index + config.min_routine_size,
                                       static_cast<uint32_t>(queue_schedule.scheduled_passes.size()));
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
        if (config.optimize_routine_resources) {
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

void RoutineSegmentationPhase::analyze_routine_resources(ExecutionRoutine& routine) SKR_NOEXCEPT 
{
    // 这是一个可选的优化，用于预分析例程的资源需求
    // 暂时提供基础实现，后续可以扩展
    
    SKR_LOG_TRACE(u8"Analyzing resources for routine %u with %zu passes",
        routine.routine_id, routine.passes.size());
    
    // TODO: 实现详细的资源分析逻辑
    // 这里可以分析每个Pass的资源访问模式，确定input/output/temp资源
}

void RoutineSegmentationPhase::calculate_segmentation_statistics() SKR_NOEXCEPT 
{
    uint32_t total_routines = 0;
    uint32_t total_passes = 0;
    uint32_t max_routine_size = 0;
    
    for (const auto& segmented_queue : result.segmented_queues) {
        total_routines += segmented_queue.total_routines;
        total_passes += segmented_queue.total_passes;
        
        for (const auto& routine : segmented_queue.routines) {
            max_routine_size = std::max(max_routine_size, 
                                       static_cast<uint32_t>(routine.passes.size()));
        }
    }
    
    result.total_routines = total_routines;
    result.max_routine_size = max_routine_size;
    result.avg_routine_size = total_routines > 0 ? 
        static_cast<float>(total_passes) / total_routines : 0.0f;
}

uint32_t RoutineSegmentationPhase::get_pass_routine_id(PassNode* pass) const 
{
    auto it = result.pass_routine_assignments.find(pass);
    return (it != result.pass_routine_assignments.end()) ? it->second : UINT32_MAX;
}

const ExecutionRoutine* RoutineSegmentationPhase::get_routine_by_id(uint32_t routine_id) const 
{
    for (const auto& segmented_queue : result.segmented_queues) {
        for (const auto& routine : segmented_queue.routines) {
            if (routine.routine_id == routine_id) {
                return &routine;
            }
        }
    }
    return nullptr;
}

void RoutineSegmentationPhase::dump_routine_segmentation() const 
{
    SKR_LOG_INFO(u8"========== Routine Segmentation Report ==========");
    
    for (const auto& segmented_queue : result.segmented_queues) {
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
            
            if (config.optimize_routine_resources) {
                SKR_LOG_INFO(u8"    Resources: %zu in, %zu out, %zu temp",
                    routine.input_resources.size(),
                    routine.output_resources.size(),
                    routine.temp_resources.size());
            }
        }
    }
    
    SKR_LOG_INFO(u8"Total: %u routines, avg size: %.1f passes",
        result.total_routines,
        result.avg_routine_size);
    SKR_LOG_INFO(u8"===============================================");
}

void RoutineSegmentationPhase::validate_routine_segmentation() const SKR_NOEXCEPT 
{
    // 验证例程切分的正确性
    for (const auto& segmented_queue : result.segmented_queues) {
        for (const auto& routine : segmented_queue.routines) {
            if (routine.passes.is_empty()) {
                SKR_LOG_WARN(u8"Empty routine %u detected", routine.routine_id);
            }
            
            if (routine.passes.size() < config.min_routine_size) {
                SKR_LOG_WARN(u8"Routine %u size %zu below minimum %u",
                    routine.routine_id, routine.passes.size(), config.min_routine_size);
            }
        }
    }
}

void RoutineSegmentationPhase::optimize_routine_boundaries() SKR_NOEXCEPT 
{
    // 可选：优化例程边界，合并小例程或拆分大例程
    // 暂时提供空实现，后续可以扩展
    SKR_LOG_TRACE(u8"Routine boundary optimization (placeholder)");
}

} // namespace render_graph
} // namespace skr