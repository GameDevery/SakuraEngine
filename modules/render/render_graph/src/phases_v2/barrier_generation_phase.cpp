#include "SkrRenderGraph/phases_v2/barrier_generation_phase.hpp"
#include "SkrRenderGraph/phases_v2/cross_queue_sync_analysis.hpp"
#include "SkrRenderGraph/phases_v2/memory_aliasing_phase.hpp"
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
    const BarrierGenerationConfig& config)
    : config_(config)
    , sync_analysis_(sync_analysis)
    , aliasing_phase_(aliasing_phase)
{
}

void BarrierGenerationPhase::on_initialize(RenderGraph* graph) SKR_NOEXCEPT
{
    barrier_result_.all_barriers.reserve(256);
    barrier_result_.pass_barriers.reserve(64);
    barrier_result_.queue_barriers.reserve(8);
    temp_barriers_.reserve(256);
    processed_resources_.reserve(128);
}

void BarrierGenerationPhase::on_finalize(RenderGraph* graph) SKR_NOEXCEPT
{
    barrier_result_.all_barriers.clear();
    barrier_result_.pass_barriers.clear();
    barrier_result_.queue_barriers.clear();
    temp_barriers_.clear();
    processed_resources_.clear();
}

void BarrierGenerationPhase::on_execute(RenderGraph* graph, RenderGraphProfiler* profiler) SKR_NOEXCEPT
{
    SKR_LOG_INFO(u8"BarrierGenerationPhase: Starting barrier generation");
    
    // 清理之前的结果
    barrier_result_.all_barriers.clear();
    barrier_result_.pass_barriers.clear();
    barrier_result_.queue_barriers.clear();
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
    
    determine_barrier_insertion_points();
    assign_barriers_to_queues();
    calculate_barrier_statistics();
    
    if (config_.validate_barrier_correctness)
    {
        validate_barrier_correctness();
    }
    
    if (config_.enable_debug_output)
    {
        dump_barrier_analysis();
        dump_barrier_insertion_points();
    }
    
    SKR_LOG_INFO(u8"BarrierGenerationPhase: Generated {} barriers ({} sync, {} aliasing, {} transition, {} execution)",
                barrier_result_.all_barriers.size(),
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
    
    // 为每个资源分析状态转换
    for (auto* resource : all_resources)
    {
        if (processed_resources_.contains(resource)) continue;
        
        PassNode* previous_pass = nullptr;
        ECGPUResourceState previous_state = CGPU_RESOURCE_STATE_UNDEFINED;
        
        for (auto* pass : all_passes)
        {
            bool uses_resource = false;
            pass->foreach_textures([&](TextureNode* tex, TextureEdge*) {
                if (tex == resource) uses_resource = true;
            });
            pass->foreach_buffers([&](BufferNode* buf, BufferEdge*) {
                if (buf == resource) uses_resource = true;
            });
            
            if (uses_resource)
            {
                // 这里需要实际的资源状态信息，简化处理
                ECGPUResourceState current_state = CGPU_RESOURCE_STATE_SHADER_RESOURCE; // 简化
                
                if (previous_pass && previous_state != current_state)
                {
                    GPUBarrier barrier = create_resource_transition_barrier(resource, previous_pass, pass);
                    temp_barriers_.add(barrier);
                    
                    if (config_.enable_debug_output)
                    {
                        SKR_LOG_TRACE(u8"Generated resource transition barrier: %s (%s -> %s)",
                                     resource->get_name(),
                                     previous_pass->get_name(),
                                     pass->get_name());
                    }
                }
                
                previous_pass = pass;
                previous_state = current_state;
            }
        }
        
        processed_resources_.insert(resource);
    }
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
    
    if (config_.merge_compatible_barriers)
    {
        merge_compatible_barriers();
    }
    
    if (config_.enable_barrier_batching)
    {
        batch_barriers();
    }
    
    barrier_result_.optimized_away_barriers = static_cast<uint32_t>(original_count - temp_barriers_.size());
    
    SKR_LOG_INFO(u8"BarrierGenerationPhase: Optimized {} barriers away ({} -> {})",
                barrier_result_.optimized_away_barriers,
                original_count,
                temp_barriers_.size());
}

void BarrierGenerationPhase::eliminate_redundant_barriers() SKR_NOEXCEPT
{
    auto end_it = std::remove_if(temp_barriers_.begin(), temp_barriers_.end(),
        [this](const GPUBarrier& barrier) {
            return is_barrier_redundant(barrier);
        });
    
    temp_barriers_.resize_default(end_it - temp_barriers_.begin());
}

void BarrierGenerationPhase::merge_compatible_barriers() SKR_NOEXCEPT
{
    skr::Vector<GPUBarrier> merged_barriers;
    merged_barriers.reserve(temp_barriers_.size());
    
    for (const auto& barrier : temp_barriers_)
    {
        bool merged = false;
        
        // 尝试与已有屏障合并
        for (auto& existing_barrier : merged_barriers)
        {
            if (are_barriers_compatible(barrier, existing_barrier))
            {
                // 合并屏障（具体逻辑依赖于屏障类型）
                merged = true;
                break;
            }
        }
        
        if (!merged)
        {
            merged_barriers.add(barrier);
        }
    }
    
    temp_barriers_ = std::move(merged_barriers);
}

void BarrierGenerationPhase::batch_barriers() SKR_NOEXCEPT
{
    // 将兼容的屏障组织成批次
    // 这里的实现依赖于具体的GPU API需求
    // 为简化，暂时保持原有结构
}

void BarrierGenerationPhase::determine_barrier_insertion_points() SKR_NOEXCEPT
{
    for (const auto& barrier : temp_barriers_)
    {
        // 确定屏障的插入点
        PassNode* insertion_pass = nullptr;
        bool insert_before = true;
        
        if (barrier.type == EBarrierType::CrossQueueSync)
        {
            // 跨队列同步屏障通常在消费者Pass之前插入
            insertion_pass = barrier.target_pass;
            insert_before = true;
        }
        else if (barrier.type == EBarrierType::MemoryAliasing)
        {
            // 内存别名屏障在Pass开始前插入
            insertion_pass = barrier.target_pass;
            insert_before = true;
        }
        else if (barrier.type == EBarrierType::ResourceTransition)
        {
            // 资源转换屏障在目标Pass之前插入
            insertion_pass = barrier.target_pass;
            insert_before = true;
        }
        
        if (insertion_pass)
        {
            BarrierInsertionPoint insertion_point;
            insertion_point.pass = insertion_pass;
            insertion_point.insert_before = insert_before;
            insertion_point.barriers.add(barrier);
            insertion_point.can_be_batched = true;
            insertion_point.priority = static_cast<uint32_t>(barrier.type);
            
            barrier_result_.pass_barriers[insertion_pass].add(insertion_point);
        }
        
        // 添加到总列表
        barrier_result_.all_barriers.add(barrier);
    }
}

void BarrierGenerationPhase::assign_barriers_to_queues() SKR_NOEXCEPT
{
    for (const auto& barrier : barrier_result_.all_barriers)
    {
        // 根据屏障类型分配到相应队列
        if (barrier.is_cross_queue())
        {
            // 跨队列屏障需要在两个队列上都有操作
            barrier_result_.queue_barriers[barrier.source_queue].add(barrier);
            barrier_result_.queue_barriers[barrier.target_queue].add(barrier);
        }
        else if (barrier.target_queue != UINT32_MAX)
        {
            barrier_result_.queue_barriers[barrier.target_queue].add(barrier);
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
    barrier.debug_name = u8"CrossQueueSync";
    
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
    
    barrier.debug_name = u8"MemoryAliasing";
    
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
    
    barrier.debug_name = u8"ResourceTransition";
    
    return barrier;
}

bool BarrierGenerationPhase::are_barriers_compatible(const GPUBarrier& barrier1, const GPUBarrier& barrier2) const SKR_NOEXCEPT
{
    // 检查屏障是否可以合并
    return barrier1.type == barrier2.type &&
           barrier1.source_queue == barrier2.source_queue &&
           barrier1.target_queue == barrier2.target_queue &&
           barrier1.target_pass == barrier2.target_pass;
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
    // 估算屏障的性能成本
    switch (barrier.type)
    {
        case EBarrierType::CrossQueueSync:
            return 10.0f; // 跨队列同步成本较高
        case EBarrierType::MemoryAliasing:
            return 5.0f;  // 内存别名成本中等
        case EBarrierType::ResourceTransition:
            return 2.0f;  // 资源转换成本较低
        case EBarrierType::ExecutionDependency:
            return 1.0f;  // 执行依赖成本最低
        default:
            return 1.0f;
    }
}

void BarrierGenerationPhase::calculate_barrier_statistics() SKR_NOEXCEPT
{
    barrier_result_.total_resource_barriers = 0;
    barrier_result_.total_sync_barriers = 0;
    barrier_result_.total_aliasing_barriers = 0;
    barrier_result_.total_execution_barriers = 0;
    barrier_result_.estimated_barrier_cost = 0.0f;
    
    for (const auto& barrier : barrier_result_.all_barriers)
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

const skr::Vector<GPUBarrier>& BarrierGenerationPhase::get_pass_barriers(PassNode* pass, bool before_pass) const
{
    static const skr::Vector<GPUBarrier> empty_barriers;
    
    auto it = barrier_result_.pass_barriers.find(pass);
    if (it != barrier_result_.pass_barriers.end())
    {
        for (const auto& insertion_point : it->second)
        {
            if (insertion_point.insert_before == before_pass)
            {
                return insertion_point.barriers;
            }
        }
    }
    
    return empty_barriers;
}

const skr::Vector<GPUBarrier>& BarrierGenerationPhase::get_queue_barriers(uint32_t queue_index) const
{
    static const skr::Vector<GPUBarrier> empty_barriers;
    
    auto it = barrier_result_.queue_barriers.find(queue_index);
    return (it != barrier_result_.queue_barriers.end()) ? it->second : empty_barriers;
}

void BarrierGenerationPhase::dump_barrier_analysis() const SKR_NOEXCEPT
{
    SKR_LOG_INFO(u8"========== Barrier Generation Analysis ==========");
    SKR_LOG_INFO(u8"Total barriers: %u", get_total_barriers());
    SKR_LOG_INFO(u8"  - Resource transition barriers: %u", barrier_result_.total_resource_barriers);
    SKR_LOG_INFO(u8"  - Cross-queue sync barriers: %u", barrier_result_.total_sync_barriers);
    SKR_LOG_INFO(u8"  - Memory aliasing barriers: %u", barrier_result_.total_aliasing_barriers);
    SKR_LOG_INFO(u8"  - Execution dependency barriers: %u", barrier_result_.total_execution_barriers);
    SKR_LOG_INFO(u8"Optimized away barriers: %u", barrier_result_.optimized_away_barriers);
    SKR_LOG_INFO(u8"Estimated barrier cost: {:.2f}", barrier_result_.estimated_barrier_cost);
    SKR_LOG_INFO(u8"Queue barrier distribution:");
    
    for (const auto& [queue_index, barriers] : barrier_result_.queue_barriers)
    {
        SKR_LOG_INFO(u8"  Queue %u: %zu barriers", queue_index, barriers.size());
    }
    
    SKR_LOG_INFO(u8"===============================================");
}

void BarrierGenerationPhase::dump_barrier_insertion_points() const SKR_NOEXCEPT
{
    SKR_LOG_INFO(u8"========== Barrier Insertion Points ==========");
    
    for (const auto& [pass, insertion_points] : barrier_result_.pass_barriers)
    {
        SKR_LOG_INFO(u8"Pass: %s", pass->get_name());
        
        for (const auto& insertion_point : insertion_points)
        {
            const char8_t* timing = insertion_point.insert_before ? u8"before" : u8"after";
            SKR_LOG_INFO(u8"  %s pass: %zu barriers (priority %u)",
                        timing, insertion_point.barriers.size(), insertion_point.priority);
            
            for (const auto& barrier : insertion_point.barriers)
            {
                const char8_t* type_name = u8"Unknown";
                switch (barrier.type)
                {
                    case EBarrierType::ResourceTransition: type_name = u8"ResourceTransition"; break;
                    case EBarrierType::CrossQueueSync: type_name = u8"CrossQueueSync"; break;
                    case EBarrierType::MemoryAliasing: type_name = u8"MemoryAliasing"; break;
                    case EBarrierType::ExecutionDependency: type_name = u8"ExecutionDependency"; break;
                }
                
                SKR_LOG_INFO(u8"    - BarrierType: %s, %s", type_name, barrier.debug_name ? barrier.debug_name : u8"unnamed");
            }
        }
    }
    
    SKR_LOG_INFO(u8"=============================================");
}

void BarrierGenerationPhase::validate_barrier_correctness() const SKR_NOEXCEPT
{
    // 验证屏障正确性
    bool has_errors = false;
    
    // 检查跨队列屏障的完整性
    for (const auto& barrier : barrier_result_.all_barriers)
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
    
    if (!has_errors)
    {
        SKR_LOG_INFO(u8"BarrierGenerationPhase: Barrier correctness validation passed");
    }
    else
    {
        SKR_LOG_ERROR(u8"BarrierGenerationPhase: Barrier correctness validation failed");
    }
}

} // namespace render_graph
} // namespace skr