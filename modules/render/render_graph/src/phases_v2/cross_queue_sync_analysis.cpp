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
    pass_to_queue_mapping_.reserve(128);
    cross_queue_dependencies_.reserve(64);
}

void CrossQueueSyncAnalysis::on_finalize(RenderGraph* graph) SKR_NOEXCEPT
{
    // 清理分析结果
    ssis_result_.raw_sync_points.clear();
    ssis_result_.optimized_sync_points.clear();
    pass_to_queue_mapping_.clear();
    cross_queue_dependencies_.clear();
}

void CrossQueueSyncAnalysis::on_execute(RenderGraph* graph, RenderGraphProfiler* profiler) SKR_NOEXCEPT
{
    SSIS_LOG(u8"CrossQueueSyncAnalysis: Starting SSIS analysis");
    
    // 清理上一帧数据
    ssis_result_.raw_sync_points.clear();
    ssis_result_.optimized_sync_points.clear();
    pass_to_queue_mapping_.clear();
    cross_queue_dependencies_.clear();
    
    // 构建Pass到队列的映射缓存
    const auto& queue_result = queue_schedule_.get_schedule_result();
    for (const auto& [pass, queue_index] : queue_result.pass_queue_assignments)
    {
        pass_to_queue_mapping_[pass] = queue_index;
    }
    
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
    
    // Step 4: 合并冗余屏障
    if (config_.enable_barrier_merging)
    {
        merge_redundant_barriers();
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
    SSIS_LOG(u8"CrossQueueSyncAnalysis: Applying SSIS optimization");
    
    // SSIS算法：对于每个同步点，检查是否被其他同步点"支配"
    // 如果存在支配关系，则可以移除被支配的同步点
    
    skr::FlatHashSet<size_t> redundant_indices;
    
    for (size_t i = 0; i < ssis_result_.raw_sync_points.size(); ++i)
    {
        const auto& sync_point = ssis_result_.raw_sync_points[i];
        
        // 检查这个同步点是否被其他同步点支配
        if (is_sync_point_redundant(sync_point))
        {
            redundant_indices.insert(i);
            
            SSIS_LOG(u8"    Marking sync point as redundant: %s -> %s (resource: %s)",
                          sync_point.producer_pass->get_name(),
                          sync_point.consumer_pass->get_name(),
                          sync_point.resource->get_name());
        }
    }
    
    // 复制非冗余的同步点到优化结果
    for (size_t i = 0; i < ssis_result_.raw_sync_points.size(); ++i)
    {
        if (!redundant_indices.contains(i))
        {
            ssis_result_.optimized_sync_points.add(ssis_result_.raw_sync_points[i]);
        }
    }
    
    SSIS_LOG(u8"CrossQueueSyncAnalysis: SSIS removed %u redundant sync points",
                  static_cast<uint32_t>(redundant_indices.size()));
}

bool CrossQueueSyncAnalysis::is_sync_point_redundant(const CrossQueueSyncPoint& sync_point) const SKR_NOEXCEPT
{
    // SSIS算法核心：检查是否存在"支配"这个同步点的其他同步点
    // 支配条件：存在另一个同步点，其同步路径包含当前同步点的因果关系
    
    for (const auto& other_sync : ssis_result_.raw_sync_points)
    {
        if (&other_sync == &sync_point) continue;
        
        // 检查other_sync是否支配sync_point
        // 条件1：other_sync的producer必须在sync_point的producer之前或同时执行
        // 条件2：other_sync的consumer必须在sync_point的consumer之后或同时执行
        // 条件3：必须存在从other_sync到sync_point的同步路径
        
        bool producer_dominates = false;
        bool consumer_dominates = false;
        
        // 使用逻辑拓扑顺序比较执行顺序
        uint32_t other_producer_order = dependency_analysis_.get_logical_topological_order(other_sync.producer_pass);
        uint32_t sync_producer_order = dependency_analysis_.get_logical_topological_order(sync_point.producer_pass);
        uint32_t other_consumer_order = dependency_analysis_.get_logical_topological_order(other_sync.consumer_pass);
        uint32_t sync_consumer_order = dependency_analysis_.get_logical_topological_order(sync_point.consumer_pass);
        
        producer_dominates = (other_producer_order <= sync_producer_order);
        consumer_dominates = (other_consumer_order >= sync_consumer_order);
        
        if (producer_dominates && consumer_dominates)
        {
            // 检查是否存在同步路径（简化版本：假设同队列内Pass有隐式同步）
            if (has_sync_path_between_passes(other_sync.producer_pass, sync_point.consumer_pass, sync_point))
            {
                return true; // 被支配，是冗余的
            }
        }
    }
    
    return false; // 不冗余
}

bool CrossQueueSyncAnalysis::has_sync_path_between_passes(PassNode* from_pass, PassNode* to_pass, const CrossQueueSyncPoint& excluding_sync) const SKR_NOEXCEPT
{
    // 简化实现：如果两个Pass在同一队列或有直接依赖关系，认为存在同步路径
    uint32_t from_queue = get_pass_queue_index(from_pass);
    uint32_t to_queue = get_pass_queue_index(to_pass);
    
    // 同队列内Pass有隐式同步
    if (from_queue == to_queue)
    {
        return true;
    }
    
    // 检查是否有直接的依赖关系
    const auto* to_deps = dependency_analysis_.get_pass_dependencies(to_pass);
    if (to_deps)
    {
        for (auto* dep_pass : to_deps->dependent_passes)
        {
            if (dep_pass == from_pass)
            {
                return true;
            }
        }
    }
    
    return false;
}

void CrossQueueSyncAnalysis::merge_redundant_barriers() SKR_NOEXCEPT
{
    // 简化实现：合并相邻的、作用于相同资源的屏障
    // 这里可以添加更复杂的屏障合并逻辑
    
    SSIS_LOG(u8"CrossQueueSyncAnalysis: Merging redundant barriers (simplified implementation)");
    
    // TODO: 实现更精细的屏障合并算法
    // 目前保持所有屏障不变
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
    auto it = pass_to_queue_mapping_.find(pass);
    return (it != pass_to_queue_mapping_.end()) ? it->second : 0; // 默认Graphics队列
}

bool CrossQueueSyncAnalysis::are_passes_on_different_queues(PassNode* pass1, PassNode* pass2) const SKR_NOEXCEPT
{
    return get_pass_queue_index(pass1) != get_pass_queue_index(pass2);
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