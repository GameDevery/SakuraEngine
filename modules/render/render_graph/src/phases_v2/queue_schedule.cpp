#include "SkrContainersDef/set.hpp"
#include "SkrGraphics/api.h"
#include "SkrRenderGraph/phases_v2/queue_schedule.hpp"
#include "SkrRenderGraph/phases_v2/pass_dependency_analysis.hpp"
#include "SkrRenderGraph/frontend/pass_node.hpp"
#include "SkrRenderGraph/frontend/resource_node.hpp"

#define QUEUE_SCHEDULE_LOG SKR_LOG_TRACE

namespace skr {
namespace render_graph {

QueueSchedule::QueueSchedule(const PassDependencyAnalysis& dependency_analysis, 
                                  const QueueScheduleConfig& cfg)
    : config(cfg), dependency_analysis(dependency_analysis)
{
}

QueueSchedule::~QueueSchedule() = default;

void QueueSchedule::on_execute(RenderGraph* graph, RenderGraphFrameExecutor* executor, RenderGraphProfiler* profiler) SKR_NOEXCEPT
{
    SkrZoneScopedN("QueueSchedule");
    QUEUE_SCHEDULE_LOG(u8"QueueSchedule: Starting timeline scheduling and fence allocation");
    
    query_queue_capabilities(graph);
    
    assign_passes_to_queues(graph);
    
    // 可选：输出调试信息
    if (config.enable_debug_output) {
        dump_timeline_result(u8"Timeline Schedule", schedule_result);
    }
}

void QueueSchedule::query_queue_capabilities(RenderGraph* graph) SKR_NOEXCEPT
{
    // 清空队列信息（简化为单一数组）
    uint32_t queue_index = 0;
    
    // 添加Graphics队列（总是存在）
    if (auto gfx_queue = graph->get_gfx_queue()) {
        all_queues.add(QueueInfo{
            .type = ERenderGraphQueueType::Graphics,
            .index = queue_index++,
            .handle = gfx_queue,
            .supports_graphics = true,
            .supports_compute = true,
            .supports_copy = true,
            .supports_present = true
        });
    }
    
    // 添加AsyncCompute队列（多个）
    if (config.enable_async_compute) {
        const auto& cmpt_queues = graph->get_cmpt_queues();
        uint32_t max_compute = skr::min((uint32_t)cmpt_queues.size(), config.max_async_compute_queues);
        for (uint32_t i = 0; i < max_compute; ++i) {
            all_queues.add(QueueInfo{
                .type = ERenderGraphQueueType::AsyncCompute,
                .index = queue_index++,
                .handle = cmpt_queues[i],
                .supports_compute = true,
                .supports_copy = true
            });
        }
    }
    
    // 添加Copy队列（多个）
    if (config.enable_copy_queue) {
        const auto& cpy_queues = graph->get_cpy_queues();
        uint32_t max_copy = skr::min((uint32_t)cpy_queues.size(), config.max_copy_queues);
        for (uint32_t i = 0; i < max_copy; ++i) {
            all_queues.add(QueueInfo{
                .type = ERenderGraphQueueType::Copy,
                .index = queue_index++,
                .handle = cpy_queues[i],
                .supports_copy = true
            });
        }
    }
    
    // 简化的调试输出
    uint32_t graphics_count = 0, compute_count = 0, copy_count = 0;
    for (const auto& queue : all_queues) {
        switch (queue.type) {
            case ERenderGraphQueueType::Graphics: graphics_count++; break;
            case ERenderGraphQueueType::AsyncCompute: compute_count++; break;
            case ERenderGraphQueueType::Copy: copy_count++; break;
            default: break;
        }
    }
    
    QUEUE_SCHEDULE_LOG(u8"QueueSchedule: Queue setup - Graphics: %d, AsyncCompute: %d, Copy: %d",
                  graphics_count, compute_count, copy_count);
}

ERenderGraphQueueType QueueSchedule::classify_pass(PassNode* pass) SKR_NOEXCEPT
{
    // 1. Present Pass必须在Graphics队列
    if (pass->pass_type == EPassType::Present) {
        return ERenderGraphQueueType::Graphics;
    }
    
    // 2. Render Pass必须在Graphics队列
    if (pass->pass_type == EPassType::Render) {
        return ERenderGraphQueueType::Graphics;
    }
    
    // 3. Copy Pass优先Copy队列（如果标记为可独立执行）
    if (pass->pass_type == EPassType::Copy && config.enable_copy_queue) {
        auto* copy_pass = static_cast<CopyPassNode*>(pass);
        if (copy_pass->get_can_be_lone()) {
            return ERenderGraphQueueType::Copy;
        }
    }
    
    // 4. Compute Pass仅基于手动标记
    if (pass->pass_type == EPassType::Compute && config.enable_async_compute) 
    {
        if (pass->has_flags(EPassFlags::PreferAsyncCompute)) {
            return ERenderGraphQueueType::AsyncCompute;
        }
    }
    
    // 默认分配到Graphics队列
    return ERenderGraphQueueType::Graphics;
}

void QueueSchedule::assign_passes_to_queues(RenderGraph* graph) SKR_NOEXCEPT
{
    schedule_result.all_queues = all_queues;
    schedule_result.queue_schedules.resize_default(all_queues.size());

    assign_passes_using_topology();
}

void QueueSchedule::assign_passes_using_topology() SKR_NOEXCEPT
{
    // 获取逻辑拓扑排序结果
    const auto& topology_result = dependency_analysis.get_logical_topology_result();
    
    QUEUE_SCHEDULE_LOG(u8"QueueSchedule: Using topology-based scheduling with %u dependency levels", 
                  topology_result.max_logical_dependency_depth + 1);
    
    // 按依赖级别顺序调度Pass，确保依赖正确性
    for (const auto& level : topology_result.logical_levels)
    {
        QUEUE_SCHEDULE_LOG(u8"  Processing dependency level %u with %u passes", 
                      level.level, static_cast<uint32_t>(level.passes.size()));
        
        // 在同一依赖级别内，按照拓扑顺序分配Pass到队列
        for (auto* pass : level.passes)
        {
            // 分类Pass并找到合适的队列
            ERenderGraphQueueType preferred_queue_type = classify_pass(pass);
            uint32_t target_queue_index;
            
            // 根据队列类型找到实际的队列索引
            switch (preferred_queue_type)
            {
                case ERenderGraphQueueType::Graphics:
                    target_queue_index = find_graphics_queue();
                    break;
                case ERenderGraphQueueType::AsyncCompute:
                    target_queue_index = find_least_loaded_compute_queue();
                    break;
                case ERenderGraphQueueType::Copy:
                    target_queue_index = find_copy_queue();
                    break;
                default:
                    target_queue_index = find_graphics_queue();
                    break;
            }
            
            // 将Pass添加到选定的队列
            if (target_queue_index < schedule_result.queue_schedules.size())
            {
                schedule_result.queue_schedules[target_queue_index].add(pass);
                schedule_result.pass_queue_assignments.add(pass, target_queue_index);
                
                QUEUE_SCHEDULE_LOG(u8"    Assigned pass '%s' to %s queue (index %u)", 
                              pass->get_name(),
                              get_queue_type_name(all_queues[target_queue_index].type),
                              target_queue_index);
            }
            else
            {
                SKR_LOG_ERROR(u8"QueueSchedule: Invalid queue index %u for pass '%s'", 
                              target_queue_index, pass->get_name());
            }
        }
    }
    
    QUEUE_SCHEDULE_LOG(u8"QueueSchedule: Topology-based scheduling completed");
}

uint32_t QueueSchedule::find_graphics_queue() const SKR_NOEXCEPT
{
    for (uint32_t i = 0; i < all_queues.size(); ++i) {
        if (all_queues[i].type == ERenderGraphQueueType::Graphics) {
            return i;
        }
    }
    return 0; // 应该总是有Graphics队列
}

uint32_t QueueSchedule::find_least_loaded_compute_queue() const SKR_NOEXCEPT
{
    // 简化轮询：找到第一个计算队列即可，避免复杂的负载计算
    static uint32_t next_compute_index = 0;
    
    skr::InlineVector<uint32_t, 4> compute_queues;
    for (uint32_t i = 0; i < all_queues.size(); ++i) {
        if (all_queues[i].type == ERenderGraphQueueType::AsyncCompute) {
            compute_queues.add(i);
        }
    }
    
    if (compute_queues.is_empty()) {
        return find_graphics_queue(); // 回退
    }
    
    uint32_t queue_idx = compute_queues[next_compute_index % compute_queues.size()];
    next_compute_index++;
    return queue_idx;
}

uint32_t QueueSchedule::find_copy_queue() const SKR_NOEXCEPT
{
    for (uint32_t i = 0; i < all_queues.size(); ++i) {
        if (all_queues[i].type == ERenderGraphQueueType::Copy) {
            return i;
        }
    }
    return find_graphics_queue(); // 回退
}

void QueueSchedule::dump_timeline_result(const char8_t* title, const TimelineScheduleResult& R) const SKR_NOEXCEPT
{
    SKR_LOG_INFO(u8"═══════════════════════════════════════");
    SKR_LOG_INFO(u8"%s", title);
    SKR_LOG_INFO(u8"═══════════════════════════════════════");
    
    // 打印队列调度信息
    SKR_LOG_INFO(u8"📅 Queue Schedules (%d queues):", (int)R.queue_schedules.size());
    for (size_t i = 0; i < R.queue_schedules.size(); ++i) {
        const auto& queue_schedule = R.queue_schedules[i];
        const auto& queue_info = R.all_queues[i];
        const char8_t* queue_name = get_queue_type_name(queue_info.type);
        
        // 为多队列类型添加索引标识
        skr::String queue_display_name;
        if (queue_info.type == ERenderGraphQueueType::AsyncCompute && all_queues.size() > 1) {
            uint32_t index = all_queues.find_if([i](auto q) { return q.index == i; }).index();
            queue_display_name = skr::format(u8"{}#{}", queue_name, index);
        } else {
            queue_display_name = skr::String(queue_name);
        }
        
        SKR_LOG_INFO(u8"  [%zu] %s Queue (index=%d, %d passes):", 
                     i, queue_display_name.c_str(), queue_info.index, (int)queue_schedule.size());
        
        for (size_t j = 0; j < queue_schedule.size(); ++j) {
            auto* pass = queue_schedule[j];
            const char8_t* pass_type_name = u8"Unknown";
            
            switch (pass->pass_type) {
                case EPassType::Render: pass_type_name = u8"Render"; break;
                case EPassType::Compute: pass_type_name = u8"Compute"; break;
                case EPassType::Copy: pass_type_name = u8"Copy"; break;
                case EPassType::Present: pass_type_name = u8"Present"; break;
                default: break;
            }
            
            SKR_LOG_INFO(u8"    [%zu] %s Pass (name=%s)", j, pass_type_name, pass->get_name());
        }
    }
    
    // 打印Pass映射统计
    SKR_LOG_INFO(u8"");
    SKR_LOG_INFO(u8"📊 Pass Assignment Summary:");
    
    uint32_t graphics_count = 0, compute_count = 0, copy_count = 0;
    for (const auto& [pass, queue_idx] : R.pass_queue_assignments) 
    {
        if (queue_idx < R.queue_schedules.size()) {
            auto queue_type = R.all_queues[queue_idx].type;
            switch (queue_type) {
                case ERenderGraphQueueType::Graphics: graphics_count++; break;
                case ERenderGraphQueueType::AsyncCompute: compute_count++; break;
                case ERenderGraphQueueType::Copy: copy_count++; break;
                default: break;
            }
        }
    }
    
    SKR_LOG_INFO(u8"  📈 Graphics Queue: %d passes", graphics_count);
    SKR_LOG_INFO(u8"  ⚡ AsyncCompute Queue: %d passes", compute_count);
    SKR_LOG_INFO(u8"  📁 Copy Queue: %d passes", copy_count);
    SKR_LOG_INFO(u8"  📝 Total Passes: %d", (int)R.pass_queue_assignments.size());
    
    SKR_LOG_INFO(u8"═══════════════════════════════════════");
}


} // namespace render_graph
} // namespace skr