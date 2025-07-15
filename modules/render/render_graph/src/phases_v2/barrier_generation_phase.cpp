#include "SkrRenderGraph/phases_v2/barrier_generation_phase.hpp"
#include "SkrRenderGraph/phases_v2/cross_queue_sync_analysis.hpp"
#include "SkrRenderGraph/phases_v2/memory_aliasing_phase.hpp"
#include "SkrRenderGraph/phases_v2/detail/hardware_constriants.hpp"
#include "SkrRenderGraph/frontend/pass_node.hpp"
#include "SkrRenderGraph/frontend/resource_node.hpp"
#include "SkrCore/log.hpp"
#include "SkrContainersDef/set.hpp"
#include <algorithm>

#define BARRIER_GENERATION_LOG SKR_LOG_DEBUG

namespace skr {
namespace render_graph {

BarrierGenerationPhase::BarrierGenerationPhase(
    const CrossQueueSyncAnalysis& sync_analysis,
    const MemoryAliasingPhase& aliasing_phase,
    const PassInfoAnalysis& pass_info_analysis,
    const ExecutionReorderPhase& reorder_phase,
    const BarrierGenerationConfig& config)
    : config_(config)
    , sync_analysis_(sync_analysis)
    , aliasing_phase_(aliasing_phase)
    , pass_info_analysis_(pass_info_analysis)
    , reorder_phase_(reorder_phase)
{
}

void BarrierGenerationPhase::on_initialize(RenderGraph* graph) SKR_NOEXCEPT
{

}

void BarrierGenerationPhase::on_finalize(RenderGraph* graph) SKR_NOEXCEPT
{

}

void BarrierGenerationPhase::on_execute(RenderGraph* graph, RenderGraphFrameExecutor* executor, RenderGraphProfiler* profiler) SKR_NOEXCEPT
{
    SkrZoneScopedN("BarrierGenerationPhase");
    BARRIER_GENERATION_LOG(u8"BarrierGenerationPhase: Starting barrier generation");
    
    // 清理之前的结果
    barrier_result_.pass_barrier_batches.clear();
    pass_barriers_.clear();
    
    // 重置统计信息
    barrier_result_.total_resource_barriers = 0;
    barrier_result_.total_sync_barriers = 0;
    barrier_result_.total_aliasing_barriers = 0;
    barrier_result_.total_execution_barriers = 0;
    barrier_result_.optimized_away_barriers = 0;
    barrier_result_.estimated_barrier_cost = 0.0f;
    
    // 核心屏障生成流程
    generate_barriers(graph);

    if (config_.enable_barrier_batch_)
    {
        batch_barriers();
    }
    else
    {
        barrier_result_.pass_barrier_batches = pass_barriers_;
    }
    
    calculate_barrier_statistics();
    
    if (config_.validate_barrier_correctness)
    {
        validate_barrier_correctness();
    }
    
    BARRIER_GENERATION_LOG(u8"BarrierGenerationPhase: Generated {} barriers ({} sync, {} aliasing, {} transition, {} execution)",
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
    
    // Count total barriers generated
    uint32_t total_barriers = 0;
    for (const auto& [pass, batches] : pass_barriers_)
    {
        for (const auto& batch : batches)
        {
            total_barriers += static_cast<uint32_t>(batch.barriers.size());
        }
    }
    BARRIER_GENERATION_LOG(u8"BarrierGenerationPhase: Generated {} preliminary barriers", total_barriers);
}

void BarrierGenerationPhase::generate_cross_queue_sync_barriers(RenderGraph* graph) SKR_NOEXCEPT
{
    const auto& ssis_result = sync_analysis_.get_ssis_result();
    
    for (const auto& sync_point : ssis_result.optimized_sync_points)
    {
        GPUBarrier barrier = create_cross_queue_barrier(sync_point);
        // Add barrier directly to pass_barriers_
        auto* execution_pass = barrier.source_pass; // Cross-queue sync barriers execute at source pass
        if (execution_pass)
        {
            auto& batch = get_or_create_barrier_batch(execution_pass, EBarrierType::CrossQueueSync);
            batch.barriers.add(barrier);
        }
        
        if (config_.enable_debug_output)
        {
            BARRIER_GENERATION_LOG(u8"Generated cross-queue sync barrier: %s -> %s (queue %u -> %u)",
                         sync_point.producer_pass ? sync_point.producer_pass->get_name() : u8"unknown",
                         sync_point.consumer_pass ? sync_point.consumer_pass->get_name() : u8"unknown",
                         sync_point.producer_queue_index,
                         sync_point.consumer_queue_index);
        }
    }
    
    BARRIER_GENERATION_LOG(u8"BarrierGenerationPhase: Generated {} cross-queue sync barriers", ssis_result.optimized_sync_points.size());
}

void BarrierGenerationPhase::generate_memory_aliasing_barriers(RenderGraph* graph) SKR_NOEXCEPT
{
    const auto& aliasing_result = aliasing_phase_.get_result();
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
        
        // Add barrier directly to pass_barriers_
        auto* execution_pass = transition.transition_pass;
        if (execution_pass)
        {
            auto& batch = get_or_create_barrier_batch(execution_pass, EBarrierType::MemoryAliasing);
            batch.barriers.add(barrier);
        }
        
        if (config_.enable_debug_output)
        {
            BARRIER_GENERATION_LOG(u8"Generated aliasing barrier at pass %s: %s -> %s (bucket %u, offset %llu)",
                         transition.transition_pass->get_name(),
                         transition.from_resource ? transition.from_resource->get_name() : u8"<initial>",
                         transition.to_resource->get_name(),
                         transition.bucket_index,
                         transition.memory_offset);
        }
    }
    
    BARRIER_GENERATION_LOG(u8"BarrierGenerationPhase: Generated %u aliasing barriers from pre-computed transitions", 
                static_cast<uint32_t>(aliasing_result.alias_transitions.size()));
}

void BarrierGenerationPhase::generate_resource_transition_barriers(RenderGraph* graph) SKR_NOEXCEPT
{
    const auto& all_resources = get_resources(graph);
    const auto& all_passes = get_passes(graph);
    
    // 获取依赖分析结果
    const auto& pass_dependency_analysis = sync_analysis_.get_dependency_analysis();
    
    // 为每个资源跟踪上一次访问的状态和Pass
    struct ResourceLastAccess {
        PassNode* last_pass = nullptr;
        ECGPUResourceState last_state = CGPU_RESOURCE_STATE_UNDEFINED;
        uint32_t last_level = UINT32_MAX;
    };
    PooledMap<ResourceNode*, ResourceLastAccess> resource_last_access;
    
    // 重要修复：按照实际的拓扑顺序处理Pass，而不是按依赖级别分组
    // 这样可以保持Pass的正确执行顺序
    const auto& logical_topological_order = pass_dependency_analysis.get_logical_topological_order();
    
    // 按拓扑顺序遍历所有Pass
    for (auto* pass : logical_topological_order)
    {
        const auto* pass_info = pass_info_analysis_.get_pass_info(pass);
        if (!pass_info)
            continue;
            
        uint32_t current_level = pass_dependency_analysis.get_logical_dependency_level(pass);
        
        // 处理这个Pass访问的所有资源
        for (const auto& access : pass_info->resource_info.all_resource_accesses)
        {
            ResourceNode* resource = access.resource;
            ECGPUResourceState current_state = access.resource_state;
            
            // 获取资源的上一次访问信息
            auto& last_access = resource_last_access.try_add_default(resource).value();
            
            // 如果这是资源的第一次访问，记录并继续
            if (last_access.last_pass == nullptr)
            {
                last_access.last_pass = pass;
                last_access.last_state = current_state;
                last_access.last_level = current_level;
                continue;
            }
            
            // 如果状态发生变化，生成转换屏障
            uint32_t source_queue = sync_analysis_.get_pass_queue_index(last_access.last_pass);
            uint32_t target_queue = sync_analysis_.get_pass_queue_index(pass);
            if ((last_access.last_state != current_state) || (source_queue != target_queue))
            {
                generate_normal_transition(resource, 
                    last_access.last_state, current_state, last_access.last_pass, pass);
            }
            
            // 更新最后访问信息
            last_access.last_pass = pass;
            last_access.last_state = current_state;
            last_access.last_level = current_level;
        }
    }
    
    BARRIER_GENERATION_LOG(u8"BarrierGenerationPhase: Generated resource transitions for %zu resources", all_resources.size());
}

void BarrierGenerationPhase::batch_barriers() SKR_NOEXCEPT
{
    barrier_result_.pass_barrier_batches = pass_barriers_;
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

const PooledVector<BarrierBatch>& BarrierGenerationPhase::get_pass_barrier_batches(PassNode* pass) const
{
    static const PooledVector<BarrierBatch> empty_batches;
    
    auto it = barrier_result_.pass_barrier_batches.find(pass);
    return it ? it.value() : empty_batches;
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
        BARRIER_GENERATION_LOG(u8"Barrier cost %.2fμs below split threshold %.2fμs, using normal barrier",
                     barrier_cost, split_barrier_threshold);
    }
    
    return cost_justified;
}

ECGPUResourceState BarrierGenerationPhase::calculate_combined_read_state(const PooledVector<ECGPUResourceState>& read_states) const SKR_NOEXCEPT
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

bool BarrierGenerationPhase::are_passes_adjacent_or_synchronized(PassNode* source_pass, PassNode* target_pass) const SKR_NOEXCEPT
{
    // 检查两个Pass是否紧挨着或者已经有同步屏障
    const uint32_t source_queue = sync_analysis_.get_pass_queue_index(source_pass);
    const uint32_t target_queue = sync_analysis_.get_pass_queue_index(target_pass);
    const auto same_queue = (source_queue == target_queue);

    // 1. 检查是否在同一队列上紧挨着执行
    if (same_queue)
    {
        const auto& timeline = reorder_phase_.get_optimized_timeline()[source_queue];
        const auto source_pass_index = timeline.find(source_pass).index();
        const auto target_pass_index = timeline.find(target_pass).index();
        if (source_pass_index == target_pass_index + 1 || source_pass_index == target_pass_index - 1)
        {
            return true;
        }
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
                BARRIER_GENERATION_LOG(u8"Skipping split barrier for %s: passes are adjacent or synchronized (%s -> %s)",
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
        
        // Add barrier directly to pass_barriers_
        auto* execution_pass = barrier.target_pass; // Normal barriers execute at target pass
        if (execution_pass)
        {
            auto& batch = get_or_create_barrier_batch(execution_pass, EBarrierType::ResourceTransition);
            batch.barriers.add(barrier);
        }
        
        if (config_.enable_debug_output)
        {
            BARRIER_GENERATION_LOG(u8"Generated normal transition for %s: %u -> %u (%s -> %s)",
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
    
    // Add Begin barrier to source pass
    {
        auto* execution_pass = source_pass; // Begin barriers execute at source pass
        if (execution_pass)
        {
            auto& batch = get_or_create_barrier_batch(execution_pass, EBarrierType::ResourceTransition);
            batch.barriers.add(begin_barrier);
        }
    }
    
    // Add End barrier to target pass
    {
        auto* execution_pass = target_pass; // End barriers execute at target pass
        if (execution_pass)
        {
            auto& batch = get_or_create_barrier_batch(execution_pass, EBarrierType::ResourceTransition);
            batch.barriers.add(end_barrier);
        }
    }
    
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
        
        BARRIER_GENERATION_LOG(u8"Generated split barriers for %s: %u -> %u (%s -> %s), estimated cost savings: %.2fμs",
                     resource->get_name(), before_state, after_state,
                     source_pass->get_name(), target_pass->get_name(),
                     normal_cost * 0.3f); // 估算30%的性能提升
    }
}

BarrierBatch& BarrierGenerationPhase::get_or_create_barrier_batch(PassNode* pass, EBarrierType batch_type) SKR_NOEXCEPT
{
    // Get or create the vector of batches for this pass
    auto& pass_batches = pass_barriers_.try_add_default(pass).value();
    
    // Find existing batch of the same type
    for (auto& batch : pass_batches)
    {
        if (batch.batch_type == batch_type)
        {
            return batch;
        }
    }
    
    // Create new batch if not found
    pass_batches.add(BarrierBatch{});
    auto& new_batch = pass_batches.back();
    new_batch.batch_type = batch_type;
    return new_batch;
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
        BARRIER_GENERATION_LOG(u8"BarrierGenerationPhase: Barrier correctness validation passed");
    }
    else
    {
        SKR_LOG_ERROR(u8"BarrierGenerationPhase: Barrier correctness validation failed");
    }
}

} // namespace render_graph
} // namespace skr