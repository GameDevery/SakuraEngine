#include "SkrRenderGraph/phases_v2/cross_queue_sync_analysis.hpp"
#include "SkrRenderGraph/phases_v2/pass_info_analysis.hpp"
#include "SkrRenderGraph/frontend/pass_node.hpp"
#include "SkrRenderGraph/frontend/resource_node.hpp"
#include "SkrCore/log.hpp"
#include <algorithm>

#define SSIS_LOG SKR_LOG_DEBUG

namespace skr {
namespace render_graph {

CrossQueueSyncAnalysis::CrossQueueSyncAnalysis(
    const PassDependencyAnalysis& dependency_analysis,
    const QueueSchedule& queue_schedule,
    const CrossQueueSyncConfig& config)
    : config_(config)
    , dependency_analysis_(dependency_analysis)
    , queue_schedule_(queue_schedule)
{
}

void CrossQueueSyncAnalysis::on_initialize(RenderGraph* graph) SKR_NOEXCEPT
{
    // 预分配容量
    ssis_result_.raw_sync_points.reserve(64);
    ssis_result_.optimized_sync_points.reserve(32);
    cross_queue_dependencies_.reserve(64);
}

void CrossQueueSyncAnalysis::on_finalize(RenderGraph* graph) SKR_NOEXCEPT
{
    // 清理分析结果
    ssis_result_.raw_sync_points.clear();
    ssis_result_.optimized_sync_points.clear();
    cross_queue_dependencies_.clear();
}

void CrossQueueSyncAnalysis::on_execute(RenderGraph* graph, RenderGraphProfiler* profiler) SKR_NOEXCEPT
{
    SSIS_LOG(u8"CrossQueueSyncAnalysis: Starting SSIS analysis");
    
    // 清理上一帧数据
    ssis_result_.raw_sync_points.clear();
    ssis_result_.optimized_sync_points.clear();
    cross_queue_dependencies_.clear();
    
    // 构建Pass到队列的映射缓存
    const auto& queue_result = queue_schedule_.get_schedule_result();
    
    // Step 1: 分析跨队列依赖关系
    analyze_cross_queue_dependencies();
    
    // Step 2: 构建原始同步点集合
    build_raw_sync_points();
    
    // Step 3: 应用SSIS优化算法
    if (config_.enable_ssis_optimization)
    {
        apply_ssis_optimization();
    }
    else
    {
        // 如果不启用SSIS，直接复制原始同步点
        ssis_result_.optimized_sync_points = ssis_result_.raw_sync_points;
    }
    
    // Step 5: 计算优化统计信息
    calculate_optimization_statistics();
    
    // 可选：输出调试信息
    if (config_.enable_debug_output)
    {
        dump_ssis_analysis();
    }
    
    SSIS_LOG(u8"CrossQueueSyncAnalysis: SSIS analysis completed - reduced %u sync points to %u (%.1f%% reduction)",
                  ssis_result_.total_raw_syncs,
                  ssis_result_.total_optimized_syncs,
                  ssis_result_.optimization_ratio * 100.0f);
}

void CrossQueueSyncAnalysis::analyze_cross_queue_dependencies() SKR_NOEXCEPT
{
    // 遍历所有Pass的依赖关系，找出跨队列的依赖
    const auto& queue_result = queue_schedule_.get_schedule_result();
    skr::Vector<PassNode*> all_passes;
    
    // 从队列调度结果中收集所有Pass
    for (const auto& queue_schedule : queue_result.queue_schedules)
    {
        for (auto* pass : queue_schedule)
        {
            all_passes.add(pass);
        }
    }
    
    for (auto* pass : all_passes)
    {
        const auto* pass_deps = dependency_analysis_.get_pass_dependencies(pass);
        if (!pass_deps) continue;
        
        uint32_t current_queue = get_pass_queue_index(pass);
        
        // 检查所有依赖的Pass
        for (auto* dep_pass : pass_deps->dependent_passes)
        {
            uint32_t dep_queue = get_pass_queue_index(dep_pass);
            
            // 如果依赖的Pass在不同队列上，记录跨队列依赖
            if (current_queue != dep_queue)
            {
                cross_queue_dependencies_.insert({dep_pass, pass});
                
                SSIS_LOG(u8"  Cross-queue dependency: %s (queue %u) -> %s (queue %u)",
                              dep_pass->get_name(), dep_queue,
                              pass->get_name(), current_queue);
            }
        }
    }
    
    SSIS_LOG(u8"CrossQueueSyncAnalysis: Found %u cross-queue dependencies",
                  static_cast<uint32_t>(cross_queue_dependencies_.size()));
}

void CrossQueueSyncAnalysis::build_raw_sync_points() SKR_NOEXCEPT
{
    // 为每个跨队列依赖创建同步点
    for (const auto& [producer_pass, consumer_pass] : cross_queue_dependencies_)
    {
        uint32_t producer_queue = get_pass_queue_index(producer_pass);
        uint32_t consumer_queue = get_pass_queue_index(consumer_pass);
        
        // 获取这两个Pass之间的资源依赖
        const auto* consumer_deps = dependency_analysis_.get_pass_dependencies(consumer_pass);
        if (!consumer_deps) continue;
        
        // 找到对producer_pass的具体资源依赖
        for (const auto& resource_dep : consumer_deps->resource_dependencies)
        {
            if (resource_dep.dependent_pass != producer_pass) continue;
            
            // 创建同步点
            CrossQueueSyncPoint sync_point;
            sync_point.type = ESyncPointType::Signal; // 默认为信号量
            sync_point.producer_queue_index = producer_queue;
            sync_point.consumer_queue_index = consumer_queue;
            sync_point.producer_pass = producer_pass;
            sync_point.consumer_pass = consumer_pass;
            sync_point.resource = resource_dep.resource;
            sync_point.from_state = resource_dep.previous_state;
            sync_point.to_state = resource_dep.current_state;
            sync_point.sync_value = 0; // 将在后续阶段分配
            
            ssis_result_.raw_sync_points.add(sync_point);
            
            SSIS_LOG(u8"    Raw sync point: %s -> %s (resource: %s, states: %d->%d)",
                          producer_pass->get_name(),
                          consumer_pass->get_name(),
                          resource_dep.resource->get_name(),
                          static_cast<int>(sync_point.from_state),
                          static_cast<int>(sync_point.to_state));
        }
    }
    
    ssis_result_.total_raw_syncs = static_cast<uint32_t>(ssis_result_.raw_sync_points.size());
    SSIS_LOG(u8"CrossQueueSyncAnalysis: Generated %u raw sync points", ssis_result_.total_raw_syncs);
}

void CrossQueueSyncAnalysis::apply_ssis_optimization() SKR_NOEXCEPT
{
    SSIS_LOG(u8"CrossQueueSyncAnalysis: Applying correct SSIS optimization algorithm");
    
    // 按照SSIS算法：为每个Pass分别优化同步点
    // 第一步：构建SSIS (Sufficient Synchronization Index Set)
    build_ssis_for_all_passes();
    
    // 第二步：为每个Pass找到最小同步点集合
    optimize_sync_points_per_pass();
    
    SSIS_LOG(u8"CrossQueueSyncAnalysis: SSIS optimization completed");
}

void CrossQueueSyncAnalysis::build_ssis_for_all_passes() SKR_NOEXCEPT
{
    SSIS_LOG(u8"CrossQueueSyncAnalysis: Building SSIS for all passes");
    
    // 获取队列调度结果
    const auto& queue_result = queue_schedule_.get_schedule_result();
    total_queue_count_ = static_cast<uint32_t>(queue_result.queue_schedules.size());
    
    // 为每个Pass计算队列内本地执行索引
    for (uint32_t queue_idx = 0; queue_idx < queue_result.queue_schedules.size(); ++queue_idx)
    {
        const auto& queue_schedule = queue_result.queue_schedules[queue_idx];
        for (uint32_t local_idx = 0; local_idx < queue_schedule.size(); ++local_idx)
        {
            PassNode* pass = queue_schedule[local_idx];
            pass_queue_local_indices_[pass] = local_idx;
        }
    }
    
    // 第一步：为每个Pass找到每个队列上最近的依赖节点
    for (const auto& [pass, queue_index] : queue_result.pass_queue_assignments)
    {
        // 初始化SSIS数组
        auto& ssis = pass_ssis_[pass];
        ssis.resize(total_queue_count_, InvalidSyncIndex);
        
        // 设置本队列的SSIS值为自己的索引
        ssis[queue_index] = pass_queue_local_indices_[pass];
        
        auto& dependencies_to_sync = pass_dependencies_to_sync_with_[pass];
        dependencies_to_sync.clear();
        
        // 查找来自其他队列的依赖
        const auto* pass_deps = dependency_analysis_.get_pass_dependencies(pass);
        if (!pass_deps) continue;
        
        // 记录每个队列上最近的依赖节点
        skr::FlatHashMap<uint32_t, PassNode*> closest_dependency_per_queue;
        
        for (const auto& resource_dep : pass_deps->resource_dependencies)
        {
            PassNode* dep_pass = resource_dep.dependent_pass;
            uint32_t dep_queue = get_pass_queue_index(dep_pass);
            
            // 只考虑跨队列依赖
            if (dep_queue == queue_index) continue;
            
            auto& closest = closest_dependency_per_queue[dep_queue];
            if (!closest || pass_queue_local_indices_[dep_pass] > pass_queue_local_indices_[closest])
            {
                closest = dep_pass;
            }
        }
        
        // 更新SSIS和依赖列表
        for (const auto& [dep_queue, closest_dep] : closest_dependency_per_queue)
        {
            ssis[dep_queue] = pass_queue_local_indices_[closest_dep];
            dependencies_to_sync.add(closest_dep);
        }
        
        SSIS_LOG(u8"    Pass %s: found %u cross-queue dependencies",
                pass->get_name(), static_cast<uint32_t>(dependencies_to_sync.size()));
    }
}

void CrossQueueSyncAnalysis::optimize_sync_points_per_pass() SKR_NOEXCEPT
{
    SSIS_LOG(u8"CrossQueueSyncAnalysis: Optimizing sync points per pass");
    
    const auto& queue_result = queue_schedule_.get_schedule_result();
    
    // 为每个Pass优化同步点
    for (const auto& [pass, queue_index] : queue_result.pass_queue_assignments)
    {
        auto deps_it = pass_dependencies_to_sync_with_.find(pass);
        if (deps_it == pass_dependencies_to_sync_with_.end() || deps_it->second.is_empty())
        {
            continue; // 没有跨队列依赖
        }
        
        auto remaining_dependencies = deps_it->second; // Copy so we can modify
        auto& pass_ssis = pass_ssis_[pass]; // Reference so we can update
        
        // 构建需要同步的队列集合
        skr::FlatHashSet<uint32_t> queues_to_sync_with;
        for (PassNode* dep : remaining_dependencies)
        {
            queues_to_sync_with.insert(get_pass_queue_index(dep));
        }
        
        // 迭代找到最小同步点集合（贪心集合覆盖算法）
        skr::Vector<PassNode*> optimal_dependencies;
        
        while (!queues_to_sync_with.empty() && !remaining_dependencies.is_empty())
        {
            uint32_t max_covered_queues = 0;
            PassNode* best_dependency = nullptr;
            skr::Vector<uint32_t> best_covered_queues;
            
            // 对每个剩余依赖节点，检查它能覆盖多少队列的同步需求
            for (PassNode* dep : remaining_dependencies)
            {
                const auto& dep_ssis = pass_ssis_[dep];
                skr::Vector<uint32_t> covered_queues;
                
                for (uint32_t queue_to_sync : queues_to_sync_with)
                {
                    uint32_t current_desired_index = pass_ssis[queue_to_sync];
                    uint32_t dep_sync_index = dep_ssis[queue_to_sync];
                    
                    // 对于同队列，减1处理（SSIS算法）
                    if (queue_to_sync == queue_index)
                    {
                        if (current_desired_index > 0) current_desired_index -= 1;
                    }
                    
                    // 检查依赖节点是否能覆盖这个队列的同步需求
                    if (dep_sync_index != InvalidSyncIndex && dep_sync_index >= current_desired_index)
                    {
                        covered_queues.add(queue_to_sync);
                    }
                }
                
                // 更新最佳选择
                if (covered_queues.size() > max_covered_queues)
                {
                    max_covered_queues = static_cast<uint32_t>(covered_queues.size());
                    best_dependency = dep;
                    best_covered_queues = covered_queues;
                }
            }
            
            // 如果找到了能覆盖队列的依赖，选择它
            if (best_dependency && max_covered_queues > 0)
            {
                // 只添加跨队列的同步（同队列的同步是隐式的）
                if (get_pass_queue_index(best_dependency) != queue_index)
                {
                    optimal_dependencies.add(best_dependency);
                }
                
                // CRITICAL: 更新Pass的SSIS值为所选依赖的SSIS值（SSIS算法关键步骤）
                const auto& best_dep_ssis = pass_ssis_[best_dependency];
                for (uint32_t covered_queue : best_covered_queues)
                {
                    pass_ssis[covered_queue] = best_dep_ssis[covered_queue];
                    queues_to_sync_with.erase(covered_queue);
                }
                
                // 从剩余依赖中移除已选择的依赖（避免重复选择）
                for (auto it = remaining_dependencies.begin(); it != remaining_dependencies.end(); ++it)
                {
                    if (*it == best_dependency)
                    {
                        remaining_dependencies.erase(it);
                        break;
                    }
                }
                
                SSIS_LOG(u8"    Pass %s: selected dependency %s (covers %u queues)",
                        pass->get_name(), best_dependency->get_name(), max_covered_queues);
            }
            else
            {
                // 无法找到覆盖剩余队列的依赖，强制退出避免死循环
                SSIS_LOG(u8"    Warning: Pass %s cannot cover remaining %u queues",
                        pass->get_name(), static_cast<uint32_t>(queues_to_sync_with.size()));
                break;
            }
        }
        
        // 基于优化后的依赖生成最终同步点
        for (PassNode* optimal_dep : optimal_dependencies)
        {
            // 在原始同步点中找到对应的同步点并添加到优化结果
            for (const auto& raw_sync : ssis_result_.raw_sync_points)
            {
                if (raw_sync.producer_pass == optimal_dep && raw_sync.consumer_pass == pass)
                {
                    ssis_result_.optimized_sync_points.add(raw_sync);
                    SSIS_LOG(u8"    Added optimized sync: %s -> %s (resource: %s)",
                            optimal_dep->get_name(), pass->get_name(), raw_sync.resource->get_name());
                    break;
                }
            }
        }
    }
    
    // 计算优化统计
    uint32_t removed_count = ssis_result_.total_raw_syncs - static_cast<uint32_t>(ssis_result_.optimized_sync_points.size());
    SSIS_LOG(u8"CrossQueueSyncAnalysis: SSIS removed %u redundant sync points", removed_count);
}

void CrossQueueSyncAnalysis::calculate_optimization_statistics() SKR_NOEXCEPT
{
    ssis_result_.total_optimized_syncs = static_cast<uint32_t>(ssis_result_.optimized_sync_points.size());
    ssis_result_.sync_reduction_count = ssis_result_.total_raw_syncs - ssis_result_.total_optimized_syncs;
    
    if (ssis_result_.total_raw_syncs > 0)
    {
        ssis_result_.optimization_ratio = static_cast<float>(ssis_result_.sync_reduction_count) / static_cast<float>(ssis_result_.total_raw_syncs);
    }
    else
    {
        ssis_result_.optimization_ratio = 0.0f;
    }
}

uint32_t CrossQueueSyncAnalysis::get_pass_queue_index(PassNode* pass) const SKR_NOEXCEPT
{
    return queue_schedule_.get_schedule_result().pass_queue_assignments.at(pass);
}

void CrossQueueSyncAnalysis::dump_ssis_analysis() const SKR_NOEXCEPT
{
    SKR_LOG_INFO(u8"========== SSIS Analysis Results ==========");
    SKR_LOG_INFO(u8"Cross-queue dependencies: %u", static_cast<uint32_t>(cross_queue_dependencies_.size()));
    SKR_LOG_INFO(u8"Raw sync points: %u", ssis_result_.total_raw_syncs);
    SKR_LOG_INFO(u8"Optimized sync points: %u", ssis_result_.total_optimized_syncs);
    SKR_LOG_INFO(u8"Sync reduction: %u (%.1f%%)", 
                 ssis_result_.sync_reduction_count,
                 ssis_result_.optimization_ratio * 100.0f);
    
    SKR_LOG_INFO(u8"\nOptimized Sync Points:");
    for (size_t i = 0; i < ssis_result_.optimized_sync_points.size(); ++i)
    {
        const auto& sync = ssis_result_.optimized_sync_points[i];
        const char8_t* type_str = (sync.type == ESyncPointType::Signal) ? u8"Signal" : u8"Wait";
        
        SKR_LOG_INFO(u8"  [%zu] %s: %s (queue %u) -> %s (queue %u) | Resource: %s | States: %d->%d",
                     i, type_str,
                     sync.producer_pass->get_name(), sync.producer_queue_index,
                     sync.consumer_pass->get_name(), sync.consumer_queue_index,
                     sync.resource->get_name(),
                     static_cast<int>(sync.from_state),
                     static_cast<int>(sync.to_state));
    }
    
    SKR_LOG_INFO(u8"==========================================");
}

void CrossQueueSyncAnalysis::dump_sync_points() const SKR_NOEXCEPT
{
    dump_ssis_analysis(); // 目前复用相同的输出
}

} // namespace render_graph
} // namespace skr