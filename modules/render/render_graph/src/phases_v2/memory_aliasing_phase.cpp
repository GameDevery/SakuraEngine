#include "SkrRenderGraph/phases_v2/memory_aliasing_phase.hpp"
#include "SkrRenderGraph/phases_v2/resource_lifetime_analysis.hpp"
#include "SkrRenderGraph/phases_v2/cross_queue_sync_analysis.hpp"
#include "SkrRenderGraph/frontend/resource_node.hpp"
#include "SkrCore/log.hpp"
#include "SkrContainersDef/set.hpp"
#include <algorithm>

namespace skr {
namespace render_graph {

bool MemoryRegion::overlaps_with(const MemoryRegion& other) const
{
    if (!is_valid() || !other.is_valid()) return false;
    return !(offset + size <= other.offset || other.offset + other.size <= offset);
}

MemoryAliasingPhase::MemoryAliasingPhase(
    const ResourceLifetimeAnalysis& lifetime_analysis,
    const CrossQueueSyncAnalysis& sync_analysis,
    const MemoryAliasingConfig& config)
    : config_(config)
    , lifetime_analysis_(lifetime_analysis)
    , sync_analysis_(sync_analysis)
{
}

void MemoryAliasingPhase::on_initialize(RenderGraph* graph) SKR_NOEXCEPT
{
    aliasing_result_.memory_buckets.reserve(config_.max_buckets);
    aliasing_result_.resource_to_bucket.reserve(128);
    aliasing_result_.resource_to_offset.reserve(128);
}

void MemoryAliasingPhase::on_finalize(RenderGraph* graph) SKR_NOEXCEPT
{
    aliasing_result_.memory_buckets.clear();
    aliasing_result_.resource_to_bucket.clear();
    aliasing_result_.resource_to_offset.clear();
    aliasing_result_.resources_need_aliasing_barrier.clear();
}

void MemoryAliasingPhase::on_execute(RenderGraph* graph, RenderGraphProfiler* profiler) SKR_NOEXCEPT
{
    SKR_LOG_INFO(u8"MemoryAliasingPhase: Starting memory aliasing analysis");
    
    if (!config_.enable_aliasing)
    {
        SKR_LOG_INFO(u8"MemoryAliasingPhase: Memory aliasing disabled");
        return;
    }
    
    // 清理之前的结果
    aliasing_result_.memory_buckets.clear();
    aliasing_result_.resource_to_bucket.clear();
    aliasing_result_.resource_to_offset.clear();
    aliasing_result_.resources_need_aliasing_barrier.clear();
    
    // 执行核心别名分析（博客算法实现）
    perform_memory_aliasing();
    calculate_aliasing_statistics();
    identify_aliasing_barriers();
    
    if (config_.enable_debug_output)
    {
        dump_aliasing_result();
        dump_memory_buckets();
    }
    
    SKR_LOG_INFO(u8"MemoryAliasingPhase: Completed. Memory reduction: {:.1f}% ({} MB -> {} MB)",
                aliasing_result_.total_compression_ratio * 100.0f,
                aliasing_result_.total_original_memory / (1024 * 1024),
                aliasing_result_.total_aliased_memory / (1024 * 1024));
}

void MemoryAliasingPhase::perform_memory_aliasing() SKR_NOEXCEPT
{
    const auto& lifetime_result = lifetime_analysis_.get_result();
    
    // 博客算法步骤1：按大小降序排序资源
    skr::Vector<ResourceNode*> sorted_resources = lifetime_result.resources_by_size_desc;
    
    // 过滤掉太小的资源
    auto filtered_end = std::remove_if(sorted_resources.begin(), sorted_resources.end(),
        [this, &lifetime_result](ResourceNode* resource) {
            auto lifetime_it = lifetime_result.resource_lifetimes.find(resource);
            return lifetime_it == lifetime_result.resource_lifetimes.end() || 
                   lifetime_it->second.memory_size < config_.min_resource_size;
        });
    sorted_resources.resize_default(filtered_end - sorted_resources.begin());
    
    if (sorted_resources.is_empty())
    {
        SKR_LOG_WARN(u8"MemoryAliasingPhase: No resources suitable for aliasing");
        return;
    }
    
    SKR_LOG_INFO(u8"MemoryAliasingPhase: Processing {} resources for aliasing", sorted_resources.size());
    
    // 博客算法步骤2：创建内存桶并尝试别名化
    create_memory_buckets();
    
    // 处理每个资源
    for (auto* resource : sorted_resources)
    {
        bool aliased = false;
        
        // 尝试放入现有桶中
        for (auto& bucket : aliasing_result_.memory_buckets)
        {
            if (try_alias_resource_in_bucket(resource, bucket))
            {
                aliased = true;
                break;
            }
        }
        
        // 如果无法放入现有桶，创建新桶
        if (!aliased && aliasing_result_.memory_buckets.size() < config_.max_buckets)
        {
            MemoryBucket new_bucket;
            auto lifetime_it = lifetime_result.resource_lifetimes.find(resource);
            if (lifetime_it != lifetime_result.resource_lifetimes.end())
            {
                new_bucket.total_size = lifetime_it->second.memory_size;
                new_bucket.used_size = lifetime_it->second.memory_size;
                new_bucket.aliased_resources.add(resource);
                new_bucket.resource_offsets[resource] = 0;
                new_bucket.original_total_size = lifetime_it->second.memory_size;
                
                uint32_t bucket_index = static_cast<uint32_t>(aliasing_result_.memory_buckets.size());
                aliasing_result_.memory_buckets.add(std::move(new_bucket));
                aliasing_result_.resource_to_bucket[resource] = bucket_index;
                aliasing_result_.resource_to_offset[resource] = 0;
                
                aliased = true;
            }
        }
        
        if (!aliased)
        {
            aliasing_result_.failed_to_alias_resources++;
            if (config_.enable_debug_output)
            {
                SKR_LOG_WARN(u8"Failed to alias resource: %s", resource->get_name());
            }
        }
        else
        {
            aliasing_result_.total_aliased_resources++;
        }
    }
}

void MemoryAliasingPhase::create_memory_buckets() SKR_NOEXCEPT
{
    // 初始化时创建空桶列表，实际桶将在处理资源时动态创建
    aliasing_result_.memory_buckets.clear();
    aliasing_result_.memory_buckets.reserve(config_.max_buckets);
}

bool MemoryAliasingPhase::try_alias_resource_in_bucket(ResourceNode* resource, MemoryBucket& bucket) SKR_NOEXCEPT
{
    const auto& lifetime_result = lifetime_analysis_.get_result();
    auto resource_lifetime_it = lifetime_result.resource_lifetimes.find(resource);
    if (resource_lifetime_it == lifetime_result.resource_lifetimes.end())
    {
        return false;
    }
    
    // 检查是否与桶中现有资源有时间冲突
    for (auto* existing_resource : bucket.aliased_resources)
    {
        if (!can_resources_alias(resource, existing_resource))
        {
            return false;
        }
    }
    
    // 博客算法核心：找到最优内存区域
    MemoryRegion optimal_region = find_optimal_memory_region(resource, bucket);
    
    if (!optimal_region.is_valid())
    {
        return false; // 没有找到合适的区域
    }
    
    // 成功找到区域，更新桶信息
    bucket.aliased_resources.add(resource);
    bucket.resource_offsets[resource] = optimal_region.offset;
    bucket.original_total_size += resource_lifetime_it->second.memory_size;
    
    // 更新桶的总大小（如果新资源超出了当前范围）
    uint64_t resource_end = optimal_region.offset + resource_lifetime_it->second.memory_size;
    if (resource_end > bucket.total_size)
    {
        bucket.total_size = resource_end;
    }
    
    // 更新全局映射
    uint32_t bucket_index = static_cast<uint32_t>(&bucket - &aliasing_result_.memory_buckets[0]);
    aliasing_result_.resource_to_bucket[resource] = bucket_index;
    aliasing_result_.resource_to_offset[resource] = optimal_region.offset;
    
    return true;
}

MemoryRegion MemoryAliasingPhase::find_optimal_memory_region(ResourceNode* resource, const MemoryBucket& bucket) SKR_NOEXCEPT
{
    const auto& lifetime_result = lifetime_analysis_.get_result();
    auto resource_lifetime_it = lifetime_result.resource_lifetimes.find(resource);
    if (resource_lifetime_it == lifetime_result.resource_lifetimes.end())
    {
        return MemoryRegion{};
    }
    
    uint64_t resource_size = resource_lifetime_it->second.memory_size;
    
    // 如果桶为空，直接返回起始位置
    if (bucket.aliased_resources.is_empty())
    {
        return MemoryRegion{ 0, resource_size };
    }
    
    // 博客算法：找到所有可别名的内存区域
    skr::Vector<MemoryRegion> aliasable_regions = find_aliasable_regions(resource, bucket);
    
    MemoryRegion optimal_region{};
    
    // 选择最小适配的区域（博客建议）
    for (const auto& region : aliasable_regions)
    {
        if (region.size >= resource_size)
        {
            if (!optimal_region.is_valid() || region.size < optimal_region.size)
            {
                optimal_region = MemoryRegion{ region.offset, resource_size };
            }
        }
    }
    
    return optimal_region;
}

skr::Vector<MemoryRegion> MemoryAliasingPhase::find_aliasable_regions(ResourceNode* resource, const MemoryBucket& bucket) SKR_NOEXCEPT
{
    skr::Vector<MemoryRegion> aliasable_regions;
    
    // 博客算法核心：收集非别名化的内存偏移
    skr::Vector<MemoryOffset> offsets;
    collect_non_aliasable_offsets(resource, bucket, offsets);
    
    if (offsets.is_empty())
    {
        // 没有冲突，整个桶都可用
        aliasable_regions.add(MemoryRegion{ 0, bucket.total_size });
        return aliasable_regions;
    }
    
    // 排序偏移点
    std::sort(offsets.begin(), offsets.end());
    
    // 博客算法：使用重叠计数器找到自由区域
    int64_t overlap_counter = 0;
    
    for (size_t i = 0; i < offsets.size() - 1; ++i)
    {
        const auto& current_offset = offsets[i];
        const auto& next_offset = offsets[i + 1];
        
        // 更新重叠计数器
        overlap_counter += (current_offset.type == EMemoryOffsetType::Start) ? 1 : -1;
        overlap_counter = std::max(overlap_counter, 0ll);
        
        // 检查是否到达可别名区域
        bool reached_aliasable_region = 
            overlap_counter == 0 && 
            current_offset.type == EMemoryOffsetType::End && 
            next_offset.type == EMemoryOffsetType::Start;
        
        if (reached_aliasable_region)
        {
            uint64_t region_start = current_offset.offset;
            uint64_t region_size = next_offset.offset - current_offset.offset;
            
            if (region_size > 0)
            {
                aliasable_regions.add(MemoryRegion{ region_start, region_size });
            }
        }
    }
    
    return aliasable_regions;
}

void MemoryAliasingPhase::collect_non_aliasable_offsets(ResourceNode* resource, const MemoryBucket& bucket, 
                                                       skr::Vector<MemoryOffset>& offsets) SKR_NOEXCEPT
{
    offsets.clear();
    
    // 添加桶边界
    offsets.add(MemoryOffset{ 0, EMemoryOffsetType::End, nullptr });
    offsets.add(MemoryOffset{ bucket.total_size, EMemoryOffsetType::Start, nullptr });
    
    // 检查与桶中现有资源的冲突
    for (auto* existing_resource : bucket.aliased_resources)
    {
        if (resources_conflict_in_time(resource, existing_resource))
        {
            auto offset_it = bucket.resource_offsets.find(existing_resource);
            if (offset_it != bucket.resource_offsets.end())
            {
                const auto& lifetime_result = lifetime_analysis_.get_result();
                auto existing_lifetime_it = lifetime_result.resource_lifetimes.find(existing_resource);
                if (existing_lifetime_it != lifetime_result.resource_lifetimes.end())
                {
                    uint64_t start_offset = offset_it->second;
                    uint64_t end_offset = start_offset + existing_lifetime_it->second.memory_size;
                    
                    offsets.add(MemoryOffset{ start_offset, EMemoryOffsetType::Start, existing_resource });
                    offsets.add(MemoryOffset{ end_offset, EMemoryOffsetType::End, existing_resource });
                }
            }
        }
    }
}

bool MemoryAliasingPhase::resources_conflict_in_time(ResourceNode* res1, ResourceNode* res2) const SKR_NOEXCEPT
{
    if (res1 == res2) return true;
    
    const auto& lifetime_result = lifetime_analysis_.get_result();
    auto lifetime1_it = lifetime_result.resource_lifetimes.find(res1);
    auto lifetime2_it = lifetime_result.resource_lifetimes.find(res2);
    
    if (lifetime1_it == lifetime_result.resource_lifetimes.end() || 
        lifetime2_it == lifetime_result.resource_lifetimes.end()) return true; // 保守处理
    
    // 使用博客建议：dependency level indices进行冲突检测
    return lifetime1_it->second.conflicts_with(lifetime2_it->second);
}

bool MemoryAliasingPhase::can_resources_alias(ResourceNode* res1, ResourceNode* res2) const SKR_NOEXCEPT
{
    // 基本检查
    if (!resources_conflict_in_time(res1, res2))
    {
        // 如果不支持跨队列别名，检查队列兼容性
        if (!config_.enable_cross_queue_aliasing)
        {
            const auto& lifetime_result = lifetime_analysis_.get_result();
            auto lifetime1_it = lifetime_result.resource_lifetimes.find(res1);
            auto lifetime2_it = lifetime_result.resource_lifetimes.find(res2);
            
            if (lifetime1_it != lifetime_result.resource_lifetimes.end() && 
                lifetime2_it != lifetime_result.resource_lifetimes.end())
            {
                // 如果资源在不同队列使用，不允许别名
                if (lifetime1_it->second.primary_queue != lifetime2_it->second.primary_queue)
                {
                    return false;
                }
            }
        }
        
        return true;
    }
    
    return false;
}

void MemoryAliasingPhase::calculate_aliasing_statistics() SKR_NOEXCEPT
{
    aliasing_result_.total_original_memory = 0;
    aliasing_result_.total_aliased_memory = 0;
    
    for (const auto& bucket : aliasing_result_.memory_buckets)
    {
        aliasing_result_.total_original_memory += bucket.original_total_size;
        aliasing_result_.total_aliased_memory += bucket.total_size;
        
        // 计算每个桶的压缩比
        const_cast<MemoryBucket&>(bucket).compression_ratio = 
            1.0f - (static_cast<float>(bucket.total_size) / static_cast<float>(bucket.original_total_size));
    }
    
    // 计算总体压缩比
    if (aliasing_result_.total_original_memory > 0)
    {
        aliasing_result_.total_compression_ratio = 
            1.0f - (static_cast<float>(aliasing_result_.total_aliased_memory) / 
                   static_cast<float>(aliasing_result_.total_original_memory));
    }
}

void MemoryAliasingPhase::identify_aliasing_barriers() SKR_NOEXCEPT
{
    // 任何在同一桶中但有多个资源的情况都需要别名屏障
    for (const auto& bucket : aliasing_result_.memory_buckets)
    {
        if (bucket.aliased_resources.size() > 1)
        {
            // 所有在这个桶中的资源都需要别名屏障
            for (auto* resource : bucket.aliased_resources)
            {
                aliasing_result_.resources_need_aliasing_barrier.insert(resource);
            }
        }
    }
}

uint32_t MemoryAliasingPhase::get_resource_bucket(ResourceNode* resource) const
{
    auto it = aliasing_result_.resource_to_bucket.find(resource);
    return (it != aliasing_result_.resource_to_bucket.end()) ? it->second : UINT32_MAX;
}

uint64_t MemoryAliasingPhase::get_resource_offset(ResourceNode* resource) const
{
    auto it = aliasing_result_.resource_to_offset.find(resource);
    return (it != aliasing_result_.resource_to_offset.end()) ? it->second : 0;
}

bool MemoryAliasingPhase::needs_aliasing_barrier(ResourceNode* resource) const
{
    return aliasing_result_.resources_need_aliasing_barrier.contains(resource);
}

uint64_t MemoryAliasingPhase::get_memory_savings() const
{
    return aliasing_result_.total_original_memory - aliasing_result_.total_aliased_memory;
}

void MemoryAliasingPhase::dump_aliasing_result() const SKR_NOEXCEPT
{
    SKR_LOG_INFO(u8"========== Memory Aliasing Results ==========");
    SKR_LOG_INFO(u8"Memory buckets: %zu", aliasing_result_.memory_buckets.size());
    SKR_LOG_INFO(u8"Aliased resources: %u", aliasing_result_.total_aliased_resources);
    SKR_LOG_INFO(u8"Failed to alias: %u", aliasing_result_.failed_to_alias_resources);
    SKR_LOG_INFO(u8"Original memory: %llu MB", aliasing_result_.total_original_memory / (1024 * 1024));
    SKR_LOG_INFO(u8"Aliased memory: %llu MB", aliasing_result_.total_aliased_memory / (1024 * 1024));
    SKR_LOG_INFO(u8"Memory savings: %llu MB ({:.1f}%)", 
                get_memory_savings() / (1024 * 1024),
                aliasing_result_.total_compression_ratio * 100.0f);
    SKR_LOG_INFO(u8"Resources needing barriers: %zu", aliasing_result_.resources_need_aliasing_barrier.size());
    SKR_LOG_INFO(u8"============================================");
}

void MemoryAliasingPhase::dump_memory_buckets() const SKR_NOEXCEPT
{
    SKR_LOG_INFO(u8"========== Memory Bucket Details ==========");
    
    for (size_t i = 0; i < aliasing_result_.memory_buckets.size(); ++i)
    {
        const auto& bucket = aliasing_result_.memory_buckets[i];
        
        SKR_LOG_INFO(u8"Bucket %zu:", i);
        SKR_LOG_INFO(u8"  Total size: %llu KB", bucket.total_size / 1024);
        SKR_LOG_INFO(u8"  Original size: %llu KB", bucket.original_total_size / 1024);
        SKR_LOG_INFO(u8"  Compression: %f%", bucket.compression_ratio * 100.0f);
        SKR_LOG_INFO(u8"  Resources (%zu):", bucket.aliased_resources.size());
        
        for (auto* resource : bucket.aliased_resources)
        {
            auto offset_it = bucket.resource_offsets.find(resource);
            if (offset_it != bucket.resource_offsets.end())
            {
                const auto& lifetime_result = lifetime_analysis_.get_result();
                auto lifetime_it = lifetime_result.resource_lifetimes.find(resource);
                if (lifetime_it != lifetime_result.resource_lifetimes.end())
                {
                    SKR_LOG_INFO(u8"    - %s: offset %llu, size %llu KB, levels [%u-%u]",
                                resource->get_name(),
                                offset_it->second,
                                lifetime_it->second.memory_size / 1024,
                                lifetime_it->second.start_dependency_level,
                                lifetime_it->second.end_dependency_level);
                }
            }
        }
    }
    
    SKR_LOG_INFO(u8"==========================================");
}

} // namespace render_graph
} // namespace skr