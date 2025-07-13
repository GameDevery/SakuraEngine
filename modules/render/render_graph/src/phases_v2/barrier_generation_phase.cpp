#include "SkrRenderGraph/phases_v2/barrier_generation_phase.hpp"
#include "SkrRenderGraph/phases_v2/cross_queue_sync_analysis.hpp"
#include "SkrRenderGraph/phases_v2/memory_aliasing_phase.hpp"
#include "SkrRenderGraph/phases_v2/detail/hardware_constriants.hpp"
#include "SkrRenderGraph/frontend/pass_node.hpp"
#include "SkrRenderGraph/frontend/resource_node.hpp"
#include "SkrCore/log.hpp"
#include "SkrContainersDef/set.hpp"
#include <algorithm>

namespace skr {
namespace render_graph {

BarrierGenerationPhase::BarrierGenerationPhase(
    const CrossQueueSyncAnalysis& sync_analysis,
    const MemoryAliasingPhase& aliasing_phase,
    const PassInfoAnalysis& pass_info_analysis,
    const BarrierGenerationConfig& config)
    : config_(config)
    , sync_analysis_(sync_analysis)
    , aliasing_phase_(aliasing_phase)
    , pass_info_analysis_(pass_info_analysis)
{
}

void BarrierGenerationPhase::on_initialize(RenderGraph* graph) SKR_NOEXCEPT
{
    temp_barriers_.reserve(256);
    processed_resources_.reserve(128);
}

void BarrierGenerationPhase::on_finalize(RenderGraph* graph) SKR_NOEXCEPT
{
    temp_barriers_.clear();
    processed_resources_.clear();
}

void BarrierGenerationPhase::on_execute(RenderGraph* graph, RenderGraphProfiler* profiler) SKR_NOEXCEPT
{
    SKR_LOG_INFO(u8"BarrierGenerationPhase: Starting barrier generation");
    
    // 清理之前的结果
    barrier_result_.pass_barrier_batches.clear();
    temp_barriers_.clear();
    processed_resources_.clear();
    
    // 重置统计信息
    barrier_result_.total_resource_barriers = 0;
    barrier_result_.total_sync_barriers = 0;
    barrier_result_.total_aliasing_barriers = 0;
    barrier_result_.total_execution_barriers = 0;
    barrier_result_.optimized_away_barriers = 0;
    barrier_result_.estimated_barrier_cost = 0.0f;
    
    // 核心屏障生成流程
    generate_barriers(graph);
    
    if (config_.enable_barrier_optimization)
    {
        optimize_barriers();
    }
    
    calculate_barrier_statistics();
    
    if (config_.validate_barrier_correctness)
    {
        validate_barrier_correctness();
    }
    
    if (config_.enable_debug_output)
    {
        dump_barrier_analysis();
    }
    
    SKR_LOG_INFO(u8"BarrierGenerationPhase: Generated {} barriers ({} sync, {} aliasing, {} transition, {} execution)",
                get_total_barriers(),
                barrier_result_.total_sync_barriers,
                barrier_result_.total_aliasing_barriers,
                barrier_result_.total_resource_barriers,
                barrier_result_.total_execution_barriers);
}

void BarrierGenerationPhase::generate_barriers(RenderGraph* graph) SKR_NOEXCEPT
{
    // 1. 从SSIS分析生成跨队列同步屏障
    generate_cross_queue_sync_barriers(graph);
    
    // 2. 从内存别名分析生成别名屏障
    generate_memory_aliasing_barriers(graph);
    
    // 3. 生成资源状态转换屏障
    generate_resource_transition_barriers(graph);
    
    // 4. 生成执行依赖屏障
    generate_execution_dependency_barriers(graph);
    
    SKR_LOG_INFO(u8"BarrierGenerationPhase: Generated {} preliminary barriers", temp_barriers_.size());
}

void BarrierGenerationPhase::generate_cross_queue_sync_barriers(RenderGraph* graph) SKR_NOEXCEPT
{
    const auto& ssis_result = sync_analysis_.get_ssis_result();
    
    for (const auto& sync_point : ssis_result.optimized_sync_points)
    {
        GPUBarrier barrier = create_cross_queue_barrier(sync_point);
        temp_barriers_.add(barrier);
        
        if (config_.enable_debug_output)
        {
            SKR_LOG_TRACE(u8"Generated cross-queue sync barrier: %s -> %s (queue %u -> %u)",
                         sync_point.producer_pass ? sync_point.producer_pass->get_name() : u8"unknown",
                         sync_point.consumer_pass ? sync_point.consumer_pass->get_name() : u8"unknown",
                         sync_point.producer_queue_index,
                         sync_point.consumer_queue_index);
        }
    }
    
    SKR_LOG_INFO(u8"BarrierGenerationPhase: Generated {} cross-queue sync barriers", ssis_result.optimized_sync_points.size());
}

void BarrierGenerationPhase::generate_memory_aliasing_barriers(RenderGraph* graph) SKR_NOEXCEPT
{
    const auto& aliasing_result = aliasing_phase_.get_result();
    
    // 新架构：直接使用 MemoryAliasingPhase 预计算的转换点
    // 这些转换点已经包含了所有必要的信息：
    // - 在哪个Pass发生转换
    // - 从哪个资源到哪个资源
    // - 内存桶信息
    
    for (const auto& transition : aliasing_result.alias_transitions)
    {
        if (!transition.transition_pass || !transition.to_resource)
            continue;
            
        // 创建别名屏障
        GPUBarrier barrier = create_aliasing_barrier(transition.to_resource, transition.transition_pass);
        
        // 设置转换特定信息
        barrier.previous_resource = transition.from_resource;
        barrier.memory_bucket_index = transition.bucket_index;
        barrier.memory_offset = transition.memory_offset;
        barrier.memory_size = transition.memory_size;
        
        temp_barriers_.add(barrier);
        
        if (config_.enable_debug_output)
        {
            SKR_LOG_TRACE(u8"Generated aliasing barrier at pass %s: %s -> %s (bucket %u, offset %llu)",
                         transition.transition_pass->get_name(),
                         transition.from_resource ? transition.from_resource->get_name() : u8"<initial>",
                         transition.to_resource->get_name(),
                         transition.bucket_index,
                         transition.memory_offset);
        }
    }
    
    SKR_LOG_INFO(u8"BarrierGenerationPhase: Generated %u aliasing barriers from pre-computed transitions", 
                static_cast<uint32_t>(aliasing_result.alias_transitions.size()));
}

void BarrierGenerationPhase::generate_resource_transition_barriers(RenderGraph* graph) SKR_NOEXCEPT
{
    const auto& all_passes = get_passes(graph);
    const auto& all_resources = get_resources(graph);
    
    // 资源状态跟踪映射: resource -> (pass -> state)
    skr::FlatHashMap<ResourceNode*, skr::FlatHashMap<PassNode*, ECGPUResourceState>> resource_states;
    
    // 跨队列读取检测映射: resource -> set<queue_indices>
    skr::FlatHashMap<ResourceNode*, skr::FlatHashSet<uint32_t>> cross_queue_accesses;
    
    // 步骤1：构建资源状态使用图
    analyze_resource_usage_patterns(graph, resource_states, cross_queue_accesses);
    
    // 步骤2：检测需要转换重路由的资源集合
    skr::FlatHashSet<ResourceNode*> resources_requiring_rerouting;
    identify_transition_rerouting_requirements(graph, cross_queue_accesses, resources_requiring_rerouting);
    
    // 步骤3：为每个资源生成状态转换屏障
    for (auto* resource : all_resources)
    {
        if (processed_resources_.contains(resource)) continue;
        
        generate_transitions_for_resource(graph, resource, resource_states[resource], resources_requiring_rerouting);
        processed_resources_.insert(resource);
    }
    
    SKR_LOG_INFO(u8"BarrierGenerationPhase: Generated resource transitions for %zu resources, %zu resources require rerouting",
                all_resources.size(), resources_requiring_rerouting.size());
}

void BarrierGenerationPhase::generate_execution_dependency_barriers(RenderGraph* graph) SKR_NOEXCEPT
{
    // 基于依赖分析生成执行依赖屏障
    // 这部分通常由PassDependencyAnalysis的结果驱动
    // 在实际实现中，这里会根据Pass间的依赖关系生成屏障
    
    auto& all_passes = get_passes(graph);
    
    for (auto* pass : all_passes)
    {
        // 这里应该查询PassDependencyAnalysis的结果
        // 为简化，暂时跳过详细实现
        // 在完整实现中，这里会分析Pass间的执行依赖并生成相应屏障
    }
}

void BarrierGenerationPhase::optimize_barriers() SKR_NOEXCEPT
{
    size_t original_count = temp_barriers_.size();
    
    if (config_.eliminate_redundant_barriers)
    {
        eliminate_redundant_barriers();
    }
    
    if (config_.enable_barrier_batching)
    {
        batch_barriers();
    }
        
    SKR_LOG_INFO(u8"BarrierGenerationPhase: Optimized {} barriers away ({} -> {})",
                barrier_result_.optimized_away_barriers,
                original_count,
                temp_barriers_.size());
}

void BarrierGenerationPhase::eliminate_redundant_barriers() SKR_NOEXCEPT
{
    const auto old_size = temp_barriers_.size();
    auto end_it = std::remove_if(temp_barriers_.begin(), temp_barriers_.end(),
        [this](const GPUBarrier& barrier) {
            return is_barrier_redundant(barrier);
        });
    
    temp_barriers_.resize_default(end_it - temp_barriers_.begin());
    barrier_result_.optimized_away_barriers = static_cast<uint32_t>(old_size - temp_barriers_.size());
}

void BarrierGenerationPhase::batch_barriers() SKR_NOEXCEPT
{
    // 将屏障按Pass组织成批次，生成 Map<PassNode*, Vector<BarrierBatch>>
    // 这是实际执行时需要的数据结构
    
    if (temp_barriers_.is_empty()) return;
    
    // 清理之前的批次结果
    barrier_result_.pass_barrier_batches.clear();
    
    // 按目标Pass分组屏障
    skr::FlatHashMap<PassNode*, skr::Vector<GPUBarrier>> barriers_by_pass;
    
    for (const auto& barrier : temp_barriers_)
    {
        if (barrier.target_pass)
        {
            barriers_by_pass[barrier.target_pass].add(barrier);
        }
    }
    
    // 为每个Pass生成屏障批次
    for (auto& [pass, pass_barriers] : barriers_by_pass)
    {
        skr::Vector<BarrierBatch>& pass_batches = barrier_result_.pass_barrier_batches[pass];
        
        if (pass_barriers.is_empty()) continue;
        
        // 按屏障类型和队列分组，然后批处理
        skr::FlatHashMap<uint64_t, skr::Vector<GPUBarrier>> grouped_barriers;
        
        for (const auto& barrier : pass_barriers)
        {
            // 生成分组键：类型 + 源队列 + 目标队列
            uint64_t group_key = (static_cast<uint64_t>(barrier.type) << 32) |
                                (static_cast<uint64_t>(barrier.source_queue) << 16) |
                                static_cast<uint64_t>(barrier.target_queue);
            
            grouped_barriers[group_key].add(barrier);
        }
        
        // 为每个分组创建批次
        uint32_t batch_index = 0;
        for (auto& [group_key, group_barriers] : grouped_barriers)
        {
            // 如果分组太大，分成多个批次
            for (size_t i = 0; i < group_barriers.size(); i += config_.max_barriers_per_batch)
            {
                BarrierBatch batch;
                batch.batch_index = batch_index++;
                
                // 复制批次中的屏障
                size_t batch_size = std::min(static_cast<size_t>(config_.max_barriers_per_batch), 
                                           group_barriers.size() - i);
                batch.barriers.reserve(batch_size);
                
                for (size_t j = 0; j < batch_size; ++j)
                {
                    const auto& barrier = group_barriers[i + j];
                    batch.barriers.add(barrier);
                    
                    // 设置批次属性（使用第一个屏障的属性）
                    if (j == 0)
                    {
                        batch.batch_type = barrier.type;
                        batch.source_queue = barrier.source_queue;
                        batch.target_queue = barrier.target_queue;
                    }
                }
                
                pass_batches.add(std::move(batch));
                
                if (config_.enable_debug_output)
                {
                    SKR_LOG_TRACE(u8"Created batch %u for pass %s with %zu barriers (type: %u)",
                                 batch.batch_index, pass->get_name(), batch_size, static_cast<uint32_t>(batch.batch_type));
                }
            }
        }
        
        if (config_.enable_debug_output)
        {
            SKR_LOG_TRACE(u8"Pass %s: %zu barriers -> %zu batches",
                         pass->get_name(), pass_barriers.size(), pass_batches.size());
        }
    }
}



GPUBarrier BarrierGenerationPhase::create_cross_queue_barrier(const CrossQueueSyncPoint& sync_point) const SKR_NOEXCEPT
{
    GPUBarrier barrier;
    barrier.type = EBarrierType::CrossQueueSync;
    barrier.source_pass = sync_point.producer_pass;
    barrier.target_pass = sync_point.consumer_pass;
    barrier.resource = sync_point.resource;
    barrier.source_queue = sync_point.producer_queue_index;
    barrier.target_queue = sync_point.consumer_queue_index;
    
    return barrier;
}

GPUBarrier BarrierGenerationPhase::create_aliasing_barrier(ResourceNode* resource, PassNode* pass) const SKR_NOEXCEPT
{
    GPUBarrier barrier;
    barrier.type = EBarrierType::MemoryAliasing;
    barrier.target_pass = pass;
    barrier.resource = resource;
    barrier.memory_offset = aliasing_phase_.get_resource_offset(resource);
    
    // 从资源获取大小信息
    const auto& lifetime_result = aliasing_phase_.get_result();
    // 这里需要从lifetime analysis获取内存大小，简化处理
    barrier.memory_size = 1024; // 简化
    
    
    return barrier;
}

GPUBarrier BarrierGenerationPhase::create_resource_transition_barrier(ResourceNode* resource, PassNode* from_pass, PassNode* to_pass) const SKR_NOEXCEPT
{
    GPUBarrier barrier;
    barrier.type = EBarrierType::ResourceTransition;
    barrier.source_pass = from_pass;
    barrier.target_pass = to_pass;
    barrier.resource = resource;
    
    // 这里需要实际的状态查询，简化处理
    barrier.before_state = CGPU_RESOURCE_STATE_SHADER_RESOURCE;
    barrier.after_state = CGPU_RESOURCE_STATE_RENDER_TARGET;
    
    
    return barrier;
}

bool BarrierGenerationPhase::is_barrier_redundant(const GPUBarrier& barrier) const SKR_NOEXCEPT
{
    // 检查屏障是否冗余
    // 简化实现：如果状态转换前后相同，则认为冗余
    if (barrier.type == EBarrierType::ResourceTransition)
    {
        return barrier.before_state == barrier.after_state;
    }
    
    return false;
}

float BarrierGenerationPhase::estimate_barrier_cost(const GPUBarrier& barrier) const SKR_NOEXCEPT
{
    using namespace HardwareConstraints;
    using namespace BarrierCosts;
    
    // 使用硬件约束常量估算屏障成本
    switch (barrier.type)
    {
        case EBarrierType::CrossQueueSync:
            return CROSS_QUEUE_SYNC; // 35.0μs - 跨队列同步成本最高
            
        case EBarrierType::MemoryAliasing:
            return L2_CACHE_FLUSH;   // 15.0μs - 内存别名需要缓存刷新
            
        case EBarrierType::ResourceTransition:
        {
            // 根据状态转换类型估算成本
            bool is_format_change = (barrier.before_state & CGPU_RESOURCE_STATE_RENDER_TARGET) &&
                                   (barrier.after_state & CGPU_RESOURCE_STATE_SHADER_RESOURCE);
            
            if (is_format_change)
                return FORMAT_CONVERSION; // 100.0μs - 格式转换
            else if (barrier.is_cross_queue())
                return L1_CACHE_FLUSH;     // 7.5μs - 跨队列资源转换
            else
                return SIMPLE_BARRIER;     // 2.5μs - 简单资源屏障
        }
        
        case EBarrierType::ExecutionDependency:
            return SIMPLE_BARRIER;         // 2.5μs - 执行依赖
            
        default:
            return SIMPLE_BARRIER;
    }
}

void BarrierGenerationPhase::calculate_barrier_statistics() SKR_NOEXCEPT
{
    barrier_result_.total_resource_barriers = 0;
    barrier_result_.total_sync_barriers = 0;
    barrier_result_.total_aliasing_barriers = 0;
    barrier_result_.total_execution_barriers = 0;
    barrier_result_.estimated_barrier_cost = 0.0f;
    
    // 从 pass_barrier_batches 计算统计信息
    for (const auto& [pass, batches] : barrier_result_.pass_barrier_batches)
    {
        for (const auto& batch : batches)
        {
            for (const auto& barrier : batch.barriers)
            {
                switch (barrier.type)
                {
                    case EBarrierType::ResourceTransition:
                        barrier_result_.total_resource_barriers++;
                        break;
                    case EBarrierType::CrossQueueSync:
                        barrier_result_.total_sync_barriers++;
                        break;
                    case EBarrierType::MemoryAliasing:
                        barrier_result_.total_aliasing_barriers++;
                        break;
                    case EBarrierType::ExecutionDependency:
                        barrier_result_.total_execution_barriers++;
                        break;
                }
                
                barrier_result_.estimated_barrier_cost += estimate_barrier_cost(barrier);
            }
        }
    }
}

const skr::Vector<BarrierBatch>& BarrierGenerationPhase::get_pass_barrier_batches(PassNode* pass) const
{
    static const skr::Vector<BarrierBatch> empty_batches;
    
    auto it = barrier_result_.pass_barrier_batches.find(pass);
    return (it != barrier_result_.pass_barrier_batches.end()) ? it->second : empty_batches;
}

uint32_t BarrierGenerationPhase::get_total_barriers() const
{
    uint32_t total = 0;
    for (const auto& [pass, batches] : barrier_result_.pass_barrier_batches)
    {
        for (const auto& batch : batches)
        {
            total += static_cast<uint32_t>(batch.barriers.size());
        }
    }
    return total;
}

uint32_t BarrierGenerationPhase::get_total_batches() const
{
    uint32_t total = 0;
    for (const auto& [pass, batches] : barrier_result_.pass_barrier_batches)
    {
        total += static_cast<uint32_t>(batches.size());
    }
    return total;
}

void BarrierGenerationPhase::dump_barrier_analysis() const SKR_NOEXCEPT
{
    SKR_LOG_INFO(u8"========== Barrier Generation Analysis ==========");
    SKR_LOG_INFO(u8"Total barriers: %u", get_total_barriers());
    SKR_LOG_INFO(u8"Total batches: %u", get_total_batches());
    SKR_LOG_INFO(u8"  - Resource transition barriers: %u", barrier_result_.total_resource_barriers);
    SKR_LOG_INFO(u8"  - Cross-queue sync barriers: %u", barrier_result_.total_sync_barriers);
    SKR_LOG_INFO(u8"  - Memory aliasing barriers: %u", barrier_result_.total_aliasing_barriers);
    SKR_LOG_INFO(u8"  - Execution dependency barriers: %u", barrier_result_.total_execution_barriers);
    SKR_LOG_INFO(u8"Optimized away barriers: %u", barrier_result_.optimized_away_barriers);
    SKR_LOG_INFO(u8"Estimated barrier cost: {:.2f}", barrier_result_.estimated_barrier_cost);
    
    SKR_LOG_INFO(u8"Pass barrier batch distribution:");
    for (const auto& [pass, batches] : barrier_result_.pass_barrier_batches)
    {
        uint32_t total_barriers_in_pass = 0;
        for (const auto& batch : batches)
        {
            total_barriers_in_pass += static_cast<uint32_t>(batch.barriers.size());
        }
        SKR_LOG_INFO(u8"  Pass %s: %zu batches, %u barriers", 
                    pass->get_name(), batches.size(), total_barriers_in_pass);
    }
    
    SKR_LOG_INFO(u8"===============================================");
}


void BarrierGenerationPhase::validate_barrier_correctness() const SKR_NOEXCEPT
{
    // 验证屏障正确性
    bool has_errors = false;
    
    // 检查跨队列屏障的完整性
    for (const auto& [pass, batches] : barrier_result_.pass_barrier_batches)
    {
        for (const auto& batch : batches)
        {
            for (const auto& barrier : batch.barriers)
            {
                if (barrier.type == EBarrierType::CrossQueueSync)
                {
                    if (barrier.source_queue == barrier.target_queue)
                    {
                        SKR_LOG_ERROR(u8"Invalid cross-queue barrier: source and target queues are the same");
                        has_errors = true;
                    }
                    
                    if (!barrier.source_pass || !barrier.target_pass)
                    {
                        SKR_LOG_ERROR(u8"Invalid cross-queue barrier: missing source or target pass");
                        has_errors = true;
                    }
                }
            }
        }
    }
    
    if (!has_errors)
    {
        SKR_LOG_INFO(u8"BarrierGenerationPhase: Barrier correctness validation passed");
    }
    else
    {
        SKR_LOG_ERROR(u8"BarrierGenerationPhase: Barrier correctness validation failed");
    }
}

// ========== 新增实现：基于博客算法的资源状态转换分析 ==========

void BarrierGenerationPhase::analyze_resource_usage_patterns(RenderGraph* graph, 
                                   skr::FlatHashMap<ResourceNode*, skr::FlatHashMap<PassNode*, ECGPUResourceState>>& resource_states,
                                   skr::FlatHashMap<ResourceNode*, skr::FlatHashSet<uint32_t>>& cross_queue_accesses) SKR_NOEXCEPT
{
    const auto& all_passes = get_passes(graph);
    
    for (auto* pass : all_passes)
    {
        const auto* pass_info = pass_info_analysis_.get_pass_info(pass);
        if (!pass_info) continue;
        
        const auto& resource_info = pass_info->resource_info;
        
        // 处理所有资源访问
        for (const auto& access : resource_info.all_resource_accesses)
        {
            auto* resource = access.resource;
            uint32_t queue_index = sync_analysis_.get_pass_queue_index(pass);
            
            // 记录资源状态
            resource_states[resource][pass] = access.resource_state;
            cross_queue_accesses[resource].insert(queue_index);
        }
    }
    
    SKR_LOG_INFO(u8"BarrierGenerationPhase: Analyzed usage patterns for %zu resources across %zu passes",
                resource_states.size(), all_passes.size());
}

void BarrierGenerationPhase::identify_transition_rerouting_requirements(RenderGraph* graph,
                                              const skr::FlatHashMap<ResourceNode*, skr::FlatHashSet<uint32_t>>& cross_queue_accesses,
                                              skr::FlatHashSet<ResourceNode*>& resources_requiring_rerouting) SKR_NOEXCEPT
{
    for (const auto& [resource, acess_queues] : cross_queue_accesses)
    {
        bool resource_needs_rerouting = false;
        
        if (acess_queues.size() > 1)
        {
            for (uint32_t queue_index : acess_queues)
            {
                if (queue_index > 0) // 非图形队列
                {
                    // 检查资源是否涉及图形专用状态
                    bool has_graphics_states = check_resource_has_graphics_states(resource);
                    if (has_graphics_states)
                    {
                        resource_needs_rerouting = true;
                        
                        if (config_.enable_debug_output)
                        {
                            SKR_LOG_TRACE(u8"Resource %s requires rerouting: queue %u cannot handle graphics states",
                                         resource->get_name(), queue_index);
                        }
                        break;
                    }
                }
            }
        }
        
        // 如果资源需要重路由，将该资源标记为需要重路由
        if (resource_needs_rerouting)
        {
            resources_requiring_rerouting.insert(resource);
        }
    }
    
    SKR_LOG_INFO(u8"BarrierGenerationPhase: %zu resources require transition rerouting", resources_requiring_rerouting.size());
}

void BarrierGenerationPhase::generate_transitions_for_resource(RenderGraph* graph, ResourceNode* resource,
                                         const skr::FlatHashMap<PassNode*, ECGPUResourceState>& pass_states,
                                         const skr::FlatHashSet<ResourceNode*>& resources_requiring_rerouting) SKR_NOEXCEPT
{
    const auto& all_passes = get_passes(graph);
    
    // 按Pass执行顺序排序状态访问
    skr::Vector<std::pair<PassNode*, ECGPUResourceState>> ordered_accesses;
    for (const auto& [pass, state] : pass_states)
    {
        ordered_accesses.add({pass, state});
    }
    
    // 简化排序：按Pass ID排序（实际应该按依赖级别排序）
    std::sort(ordered_accesses.begin(), ordered_accesses.end(),
        [](const auto& a, const auto& b) {
            return a.first->get_id() < b.first->get_id();
        });
    
    // 生成状态转换
    ECGPUResourceState previous_state = CGPU_RESOURCE_STATE_UNDEFINED;
    PassNode* previous_pass = nullptr;
    
    for (const auto& [pass, current_state] : ordered_accesses)
    {
        if (previous_pass && previous_state != current_state)
        {
            uint32_t current_queue = sync_analysis_.get_pass_queue_index(pass);
            uint32_t previous_queue = sync_analysis_.get_pass_queue_index(previous_pass);
            
            // 检查该资源是否需要重路由
            bool needs_rerouting = resources_requiring_rerouting.contains(resource);
            
            if (needs_rerouting)
            {
                // 重路由到最有能力的队列
                skr::FlatHashSet<uint32_t> involved_queues = {current_queue, previous_queue};
                uint32_t competent_queue = find_most_competent_queue(involved_queues);
                
                generate_rerouted_transition(resource, previous_state, current_state, competent_queue, previous_pass, pass);
            }
            else
            {
                // 正常转换
                generate_normal_transition(resource, previous_state, current_state, previous_pass, pass);
            }
        }
        
        previous_state = current_state;
        previous_pass = pass;
    }
}

uint32_t BarrierGenerationPhase::find_most_competent_queue(const skr::FlatHashSet<uint32_t>& queue_set) const SKR_NOEXCEPT
{
    // 根据博客：图形队列通常是最有能力的
    // 顺序：Graphics > Compute > Transfer
    
    for (uint32_t queue_index : queue_set)
    {
        if (queue_index == 0) // 图形队列
            return queue_index;
    }
    
    for (uint32_t queue_index : queue_set)
    {
        if (queue_index < 4) // 计算队列（假设前4个是计算）
            return queue_index;
    }
    
    // 返回第一个可用队列作为后备
    return queue_set.empty() ? 0 : *queue_set.begin();
}

bool BarrierGenerationPhase::is_state_transition_supported_on_queue(uint32_t queue_index, ECGPUResourceState before_state, ECGPUResourceState after_state) const SKR_NOEXCEPT
{
    // 简化的队列能力检查
    if (queue_index == 0) // 图形队列支持所有状态
        return true;
    
    // 计算队列不支持图形专用状态
    bool has_graphics_before = (before_state & (CGPU_RESOURCE_STATE_RENDER_TARGET | CGPU_RESOURCE_STATE_DEPTH_WRITE)) != 0;
    bool has_graphics_after = (after_state & (CGPU_RESOURCE_STATE_RENDER_TARGET | CGPU_RESOURCE_STATE_DEPTH_WRITE)) != 0;
    
    return !(has_graphics_before || has_graphics_after);
}

bool BarrierGenerationPhase::can_use_split_barriers(uint32_t transmitting_queue, uint32_t receiving_queue, ECGPUResourceState before_state, ECGPUResourceState after_state) const SKR_NOEXCEPT
{
    // 分离屏障要求两个队列都支持相关状态转换
    if (!is_state_transition_supported_on_queue(transmitting_queue, before_state, after_state) ||
        !is_state_transition_supported_on_queue(receiving_queue, before_state, after_state))
    {
        return false;
    }
    
    // 只有重量级的屏障才值得使用分离屏障优化
    // 创建临时屏障对象来估算成本
    GPUBarrier temp_barrier;
    temp_barrier.type = EBarrierType::ResourceTransition;
    temp_barrier.before_state = before_state;
    temp_barrier.after_state = after_state;
    temp_barrier.source_queue = transmitting_queue;
    temp_barrier.target_queue = receiving_queue;
    
    float barrier_cost = estimate_barrier_cost(temp_barrier);
    
    // 只有成本超过阈值的屏障才考虑分离优化
    // 使用配置中的成本阈值，默认为轻量级屏障成本的4倍以上
    const float split_barrier_threshold = config_.barrier_cost_threshold * 4.0f;
    
    bool cost_justified = barrier_cost >= split_barrier_threshold;
    
    if (config_.enable_debug_output && !cost_justified)
    {
        SKR_LOG_TRACE(u8"Barrier cost %.2fμs below split threshold %.2fμs, using normal barrier",
                     barrier_cost, split_barrier_threshold);
    }
    
    return cost_justified;
}

ECGPUResourceState BarrierGenerationPhase::calculate_combined_read_state(const skr::Vector<ECGPUResourceState>& read_states) const SKR_NOEXCEPT
{
    // 合并多个读取状态
    ECGPUResourceState combined = CGPU_RESOURCE_STATE_UNDEFINED;
    
    for (auto state : read_states)
    {
        combined = static_cast<ECGPUResourceState>(combined | state);
    }
    
    return combined;
}

ECGPUResourceState BarrierGenerationPhase::get_resource_state_for_usage(ResourceNode* resource, PassNode* pass, bool is_write) const SKR_NOEXCEPT
{
    // 使用PassInfoAnalysis的状态信息
    return pass_info_analysis_.get_resource_state(pass, resource);
}

bool BarrierGenerationPhase::check_resource_has_graphics_states(ResourceNode* resource) const SKR_NOEXCEPT
{
    // 使用PassInfoAnalysis的预计算结果检查资源是否需要图形队列
    return pass_info_analysis_.resource_requires_graphics_queue(resource);
}

bool BarrierGenerationPhase::are_passes_adjacent_or_synchronized(PassNode* source_pass, PassNode* target_pass) const SKR_NOEXCEPT
{
    // 检查两个Pass是否紧挨着或者已经有同步屏障
    
    // 1. 检查是否在同一队列上紧挨着执行
    uint32_t source_queue = sync_analysis_.get_pass_queue_index(source_pass);
    uint32_t target_queue = sync_analysis_.get_pass_queue_index(target_pass);
    auto source_pass_index = sync_analysis_.get_local_pass_index(source_pass);
    auto target_pass_index = sync_analysis_.get_local_pass_index(target_pass);

    if (source_queue == target_queue && 
        (target_pass_index == source_pass_index + 1 || target_pass_index == source_pass_index - 1))
    {
        // 同队列上的Pass天然串行，不需要分离屏障
        return true;
    }
    
    // 2. 检查是否已经有跨队列同步屏障
    const auto& ssis_result = sync_analysis_.get_ssis_result();
    for (const auto& sync_point : ssis_result.optimized_sync_points)
    {
        if (sync_point.producer_pass == source_pass && sync_point.consumer_pass == target_pass)
        {
            // 已经有专门的同步点，不需要分离屏障
            return true;
        }
    }
    
    return false;
}

void BarrierGenerationPhase::generate_rerouted_transition(ResourceNode* resource, ECGPUResourceState before_state, ECGPUResourceState after_state, uint32_t competent_queue, PassNode* source_pass, PassNode* target_pass) SKR_NOEXCEPT
{
    GPUBarrier barrier;
    barrier.type = EBarrierType::ResourceTransition;
    barrier.resource = resource;
    barrier.before_state = before_state;
    barrier.after_state = after_state;
    barrier.source_pass = source_pass;
    barrier.target_pass = target_pass;
    barrier.source_queue = sync_analysis_.get_pass_queue_index(source_pass);
    barrier.target_queue = competent_queue;
    
    temp_barriers_.add(barrier);
    
    if (config_.enable_debug_output)
    {
        SKR_LOG_TRACE(u8"Generated rerouted transition for %s: %u -> %u (queue %u, %s -> %s)",
                     resource->get_name(), before_state, after_state, competent_queue,
                     source_pass->get_name(), target_pass->get_name());
    }
}

void BarrierGenerationPhase::generate_normal_transition(ResourceNode* resource, ECGPUResourceState before_state, ECGPUResourceState after_state, PassNode* source_pass, PassNode* target_pass) SKR_NOEXCEPT
{
    uint32_t source_queue = sync_analysis_.get_pass_queue_index(source_pass);
    uint32_t target_queue = sync_analysis_.get_pass_queue_index(target_pass);
    
    // 检查是否可以使用分离屏障优化
    bool should_use_split_barrier = false;
    
    if (config_.enable_split_barriers && 
        can_use_split_barriers(source_queue, target_queue, before_state, after_state))
    {
        // 进一步检查是否值得使用分离屏障
        if (!are_passes_adjacent_or_synchronized(source_pass, target_pass))
        {
            should_use_split_barrier = true;
        }
        else
        {
            if (config_.enable_debug_output)
            {
                SKR_LOG_TRACE(u8"Skipping split barrier for %s: passes are adjacent or synchronized (%s -> %s)",
                             resource->get_name(), source_pass->get_name(), target_pass->get_name());
            }
        }
    }
    
    if (should_use_split_barrier)
    {
        // 生成分离屏障
        generate_split_barrier(resource, before_state, after_state, source_pass, target_pass);
    }
    else
    {
        // 生成普通屏障
        GPUBarrier barrier;
        barrier.type = EBarrierType::ResourceTransition;
        barrier.resource = resource;
        barrier.before_state = before_state;
        barrier.after_state = after_state;
        barrier.source_pass = source_pass;
        barrier.target_pass = target_pass;
        barrier.source_queue = source_queue;
        barrier.target_queue = target_queue;
        
        temp_barriers_.add(barrier);
        
        if (config_.enable_debug_output)
        {
            SKR_LOG_TRACE(u8"Generated normal transition for %s: %u -> %u (%s -> %s)",
                         resource->get_name(), before_state, after_state,
                         source_pass->get_name(), target_pass->get_name());
        }
    }
}

void BarrierGenerationPhase::generate_split_barrier(ResourceNode* resource, ECGPUResourceState before_state, ECGPUResourceState after_state, PassNode* source_pass, PassNode* target_pass) SKR_NOEXCEPT
{
    // 分离屏障优化：将重量级状态转换分为Begin/End两部分
    // Begin在生产者方发起，End在消费者方等待，允许中间的其他工作并行执行
    
    // 生成Begin屏障（在源Pass后插入，启动状态转换）
    GPUBarrier begin_barrier;
    begin_barrier.is_begin = true; // 标记为Begin屏障
    begin_barrier.type = EBarrierType::ResourceTransition;
    begin_barrier.resource = resource;
    begin_barrier.before_state = before_state;
    begin_barrier.after_state = after_state;
    begin_barrier.source_pass = source_pass;
    begin_barrier.target_pass = target_pass; // Begin屏障在源Pass后插入，所以target_pass是source_pass
    begin_barrier.source_queue = sync_analysis_.get_pass_queue_index(source_pass);
    begin_barrier.target_queue = sync_analysis_.get_pass_queue_index(target_pass);
    
    // 生成End屏障（在目标Pass前插入，等待状态转换完成）
    GPUBarrier end_barrier;
    end_barrier.is_end = true;    // 标记为End屏障
    end_barrier.type = EBarrierType::ResourceTransition;
    end_barrier.resource = resource;
    end_barrier.before_state = before_state;
    end_barrier.after_state = after_state;
    end_barrier.source_pass = source_pass;
    end_barrier.target_pass = target_pass; // End屏障在目标Pass前插入
    end_barrier.source_queue = sync_analysis_.get_pass_queue_index(source_pass);
    end_barrier.target_queue = sync_analysis_.get_pass_queue_index(target_pass);
    
    temp_barriers_.add(begin_barrier);
    temp_barriers_.add(end_barrier);
    
    if (config_.enable_debug_output)
    {
        // 估算节省的成本
        GPUBarrier temp_normal_barrier;
        temp_normal_barrier.type = EBarrierType::ResourceTransition;
        temp_normal_barrier.before_state = before_state;
        temp_normal_barrier.after_state = after_state;
        temp_normal_barrier.source_queue = sync_analysis_.get_pass_queue_index(source_pass);
        temp_normal_barrier.target_queue = sync_analysis_.get_pass_queue_index(target_pass);
        float normal_cost = estimate_barrier_cost(temp_normal_barrier);
        
        SKR_LOG_TRACE(u8"Generated split barriers for %s: %u -> %u (%s -> %s), estimated cost savings: %.2fμs",
                     resource->get_name(), before_state, after_state,
                     source_pass->get_name(), target_pass->get_name(),
                     normal_cost * 0.3f); // 估算30%的性能提升
    }
}

} // namespace render_graph
} // namespace skr