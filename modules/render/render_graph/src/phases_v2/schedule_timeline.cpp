#include "SkrContainersDef/set.hpp"
#include "SkrGraphics/api.h"
#include "SkrRenderGraph/phases_v2/schedule_timeline.hpp"
#include "SkrRenderGraph/phases_v2/pass_dependency_analysis.hpp"
#include "SkrRenderGraph/frontend/pass_node.hpp"
#include "SkrRenderGraph/frontend/resource_node.hpp"

namespace skr {
namespace render_graph {

ScheduleTimeline::ScheduleTimeline(const PassDependencyAnalysis& dependency_analysis, const ScheduleTimelineConfig& cfg)
    : config(cfg), dependency_analysis(dependency_analysis)
{
}

ScheduleTimeline::~ScheduleTimeline() = default;

void ScheduleTimeline::on_initialize(RenderGraph* graph) SKR_NOEXCEPT
{
    // 查询队列能力
    query_queue_capabilities(graph);
    
    // 初始化调度结果
    schedule_result.queue_schedules.clear();
    schedule_result.sync_requirements.clear(); 
    schedule_result.pass_queue_assignments.clear();
}

void ScheduleTimeline::on_finalize(RenderGraph* graph) SKR_NOEXCEPT
{
    // 清理所有状态
    clear_frame_data();
}

void ScheduleTimeline::clear_frame_data() SKR_NOEXCEPT
{
    // 清理调度结果
    schedule_result.queue_schedules.clear();
    schedule_result.sync_requirements.clear();
    schedule_result.pass_queue_assignments.clear();
    
    // 清理每帧重建的数据（简化）
    dependency_info.clear();
}

void ScheduleTimeline::on_execute(RenderGraph* graph, RenderGraphProfiler* profiler) SKR_NOEXCEPT
{
    SKR_LOG_DEBUG(u8"ScheduleTimeline: Starting timeline scheduling and fence allocation");
    
    // 0. 清空上一帧的数据 - RG中的资源和Pass每帧都不稳定
    clear_frame_data();
    
    // 1. 分析Pass依赖关系
    analyze_dependencies(graph);
    
    // 2. 分类并分配Pass到队列
    assign_passes_to_queues(graph);
    
    // 4. 计算同步需求
    calculate_sync_requirements(graph);
    
    SKR_LOG_DEBUG(u8"ScheduleTimeline: Scheduling completed. Queues: %d, SyncRequirements: %d", 
                  (int)schedule_result.queue_schedules.size(), (int)schedule_result.sync_requirements.size());
}

void ScheduleTimeline::query_queue_capabilities(RenderGraph* graph) SKR_NOEXCEPT
{
    // 清空队列信息（简化为单一数组）
    all_queues.clear();
    
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
    
    SKR_LOG_DEBUG(u8"ScheduleTimeline: Queue setup - Graphics: %d, AsyncCompute: %d, Copy: %d",
                  graphics_count, compute_count, copy_count);
}

void ScheduleTimeline::analyze_dependencies(RenderGraph* graph) SKR_NOEXCEPT
{
    dependency_info.clear();
    
    auto& passes = get_passes(graph);
    
    // 使用传入的 PassDependencyAnalysis 的分析结果
    SKR_LOG_INFO(u8"ScheduleTimeline: using real dependency analysis results");
    
    // 构建Pass级别的依赖图
    for (auto* pass : passes) {
        auto& info = dependency_info[pass];
        
        // 从 PassDependencyAnalysis 结果提取依赖关系
        const PassDependencies* pass_deps = dependency_analysis.get_pass_dependencies(pass);
        if (pass_deps != nullptr)
        {
            // 收集此 Pass 依赖的所有前置 Pass
            skr::InlineSet<PassNode*, 4> unique_dependencies;
            
            for (const auto& resource_dep : pass_deps->resource_dependencies)
            {
                unique_dependencies.add(resource_dep.dependent_pass);
            }
            
            // 添加到依赖信息中
            for (PassNode* dep_pass : unique_dependencies)
            {
                info.dependencies.add(dep_pass);
                dependency_info[dep_pass].dependents.add(pass);
            }
        }
    }
    
    // 输出调试信息
    dependency_analysis.dump_dependencies();
    
    SKR_LOG_INFO(u8"ScheduleTimeline: dependency analysis completed for {} passes", passes.size());
}


ERenderGraphQueueType ScheduleTimeline::classify_pass(PassNode* pass) SKR_NOEXCEPT
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

// 注意：移除了基于Pass类型的队列能力推断方法
// 现在队列分配完全基于手动标记：
// - Present/Render Pass -> Graphics队列
// - Copy Pass with can_be_lone() -> Copy队列  
// - Compute Pass with PreferAsyncCompute flag -> AsyncCompute队列
// - 其他 -> Graphics队列

void ScheduleTimeline::assign_passes_to_queues(RenderGraph* graph) SKR_NOEXCEPT
{
    // 初始化队列调度结果（简化）
    schedule_result.queue_schedules.clear();
    schedule_result.queue_schedules.reserve(all_queues.size());
    for (uint32_t i = 0; i < all_queues.size(); ++i) {
        const auto& queue = all_queues[i];
        schedule_result.queue_schedules.add(QueueScheduleInfo{queue.type, i});
    }
    
    // 直接遍历Pass进行分类和分配（简化的队列选择）
    auto& passes = get_passes(graph);
    for (auto* pass : passes) {
        // 直接分类并选择队列（使用简化的队列查找）
        auto preferred_type = classify_pass(pass);
        uint32_t queue_idx = find_graphics_queue(); // 默认回退
        
        // 根据偏好类型选择队列（简化逻辑）
        switch (preferred_type) {
            case ERenderGraphQueueType::AsyncCompute:
                queue_idx = find_least_loaded_compute_queue();
                break;
            case ERenderGraphQueueType::Copy:
                queue_idx = find_copy_queue();
                break;
            case ERenderGraphQueueType::Graphics:
            default:
                queue_idx = find_graphics_queue();
                break;
        }
        
        // 分配到队列（简化，只保留一个映射）
        schedule_result.queue_schedules[queue_idx].scheduled_passes.add(pass);
        schedule_result.pass_queue_assignments[pass] = queue_idx;
    }
}

uint32_t ScheduleTimeline::find_graphics_queue() const SKR_NOEXCEPT
{
    for (uint32_t i = 0; i < all_queues.size(); ++i) {
        if (all_queues[i].type == ERenderGraphQueueType::Graphics) {
            return i;
        }
    }
    return 0; // 应该总是有Graphics队列
}

uint32_t ScheduleTimeline::find_least_loaded_compute_queue() const SKR_NOEXCEPT
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

uint32_t ScheduleTimeline::find_copy_queue() const SKR_NOEXCEPT
{
    for (uint32_t i = 0; i < all_queues.size(); ++i) {
        if (all_queues[i].type == ERenderGraphQueueType::Copy) {
            return i;
        }
    }
    return find_graphics_queue(); // 回退
}

void ScheduleTimeline::calculate_sync_requirements(RenderGraph* graph) SKR_NOEXCEPT
{
    schedule_result.sync_requirements.clear();
    
    // 为每个跨队列依赖关系生成独立的同步需求，跳过RAR依赖
    skr::Vector<SyncRequirement> sync_requirements;
    
    for (auto& [pass, queue] : schedule_result.pass_queue_assignments) {
        // 从PassDependencyAnalysis获取详细的资源依赖信息
        const PassDependencies* pass_deps = dependency_analysis.get_pass_dependencies(pass);
        if (!pass_deps) continue;
        
        for (const auto& resource_dep : pass_deps->resource_dependencies) {
            // 跳过RAR依赖 - 多个队列可以并行读取同一资源
            if (resource_dep.dependency_type == EResourceDependencyType::RAR) {
                continue;
            }
            
            auto* dep_pass = resource_dep.dependent_pass;
            auto dep_queue_iter = schedule_result.pass_queue_assignments.find(dep_pass);
            if (dep_queue_iter != schedule_result.pass_queue_assignments.end()) {
                auto dep_queue = dep_queue_iter->second;
                if (dep_queue != queue) {
                    // 为每个跨队列依赖创建独立的同步需求
                    SyncRequirement requirement;
                    requirement.signal_queue_index = dep_queue;
                    requirement.wait_queue_index = queue;
                    requirement.signal_after_pass = dep_pass;
                    requirement.wait_before_pass = pass;
                    
                    // 检查是否已存在相同的同步需求(避免重复)
                    bool duplicate = false;
                    for (const auto& existing : sync_requirements) {
                        if (existing.signal_queue_index == requirement.signal_queue_index &&
                            existing.wait_queue_index == requirement.wait_queue_index &&
                            existing.signal_after_pass == requirement.signal_after_pass &&
                            existing.wait_before_pass == requirement.wait_before_pass) {
                            duplicate = true;
                            break;
                        }
                    }
                    
                    if (!duplicate) {
                        sync_requirements.add(requirement);
                    }
                }
            }
        }
    }
    
    // 移动到最终结果
    schedule_result.sync_requirements = std::move(sync_requirements);
}


void ScheduleTimeline::dump_timeline_result(const char8_t* title, const TimelineScheduleResult& R) const SKR_NOEXCEPT
{
    SKR_LOG_INFO(u8"═══════════════════════════════════════");
    SKR_LOG_INFO(u8"%s", title);
    SKR_LOG_INFO(u8"═══════════════════════════════════════");
    
    // 打印队列调度信息
    SKR_LOG_INFO(u8"📅 Queue Schedules (%d queues):", (int)R.queue_schedules.size());
    for (size_t i = 0; i < R.queue_schedules.size(); ++i) {
        const auto& queue_schedule = R.queue_schedules[i];
        const char8_t* queue_name = get_queue_type_name(queue_schedule.queue_type);
        
        // 为多队列类型添加索引标识
        skr::String queue_display_name;
        if (queue_schedule.queue_type == ERenderGraphQueueType::AsyncCompute && all_queues.size() > 1) {
            uint32_t index = all_queues.find_if([i](auto q) { return q.index == i; }).index();
            queue_display_name = skr::format(u8"{}#{}", queue_name, index);
        } else {
            queue_display_name = skr::String(queue_name);
        }
        
        SKR_LOG_INFO(u8"  [%zu] %s Queue (index=%d, %d passes):", 
                     i, queue_display_name.c_str(), queue_schedule.queue_index, 
                     (int)queue_schedule.scheduled_passes.size());
        
        for (size_t j = 0; j < queue_schedule.scheduled_passes.size(); ++j) {
            auto* pass = queue_schedule.scheduled_passes[j];
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
    
    // 打印同步需求
    SKR_LOG_INFO(u8"");
    SKR_LOG_INFO(u8"🔄 Sync Requirements (%d requirements):", (int)R.sync_requirements.size());
    
    if (R.sync_requirements.is_empty()) {
        SKR_LOG_INFO(u8"  ✅ No cross-queue synchronization needed");
    } else {
        for (size_t i = 0; i < R.sync_requirements.size(); ++i) {
            const auto& requirement = R.sync_requirements[i];
            
            const char8_t* signal_queue_name = u8"Unknown";
            const char8_t* wait_queue_name = u8"Unknown";
            
            if (requirement.signal_queue_index < R.queue_schedules.size()) {
                signal_queue_name = get_queue_type_name(R.queue_schedules[requirement.signal_queue_index].queue_type);
            }
            if (requirement.wait_queue_index < R.queue_schedules.size()) {
                wait_queue_name = get_queue_type_name(R.queue_schedules[requirement.wait_queue_index].queue_type);
            }
            
            SKR_LOG_INFO(u8"  [%zu] %s[%d] --signal--> %s[%d]", 
                         i, signal_queue_name, requirement.signal_queue_index,
                         wait_queue_name, requirement.wait_queue_index);
            SKR_LOG_INFO(u8"       Signal after: %s, Wait before: %s",
                         requirement.signal_after_pass->get_name(), requirement.wait_before_pass->get_name());
        }
    }
    
    // 打印Pass映射统计
    SKR_LOG_INFO(u8"");
    SKR_LOG_INFO(u8"📊 Pass Assignment Summary:");
    
    uint32_t graphics_count = 0, compute_count = 0, copy_count = 0;
    for (const auto& [pass, queue_idx] : R.pass_queue_assignments) {
        if (queue_idx < R.queue_schedules.size()) {
            auto queue_type = R.queue_schedules[queue_idx].queue_type;
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