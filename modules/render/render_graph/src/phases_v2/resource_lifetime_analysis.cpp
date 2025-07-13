#include "SkrRenderGraph/phases_v2/resource_lifetime_analysis.hpp"
#include "SkrRenderGraph/phases_v2/pass_dependency_analysis.hpp"
#include "SkrRenderGraph/phases_v2/queue_schedule.hpp"
#include "SkrRenderGraph/frontend/pass_node.hpp"
#include "SkrRenderGraph/frontend/resource_node.hpp"
#include "SkrRenderGraph/frontend/base_types.hpp"
#include "SkrCore/log.hpp"
#include "SkrContainersDef/set.hpp"
#include <algorithm>

namespace skr {
namespace render_graph {

bool ResourceLifetime::conflicts_with(const ResourceLifetime& other) const
{
    if (resource == other.resource) return false;
    
    // 使用依赖级别进行冲突检测（支持并行执行）
    // 博客核心：使用dependency level indices而不是execution indices
    bool level_conflict = !(end_dependency_level < other.start_dependency_level || 
                           start_dependency_level > other.end_dependency_level);
    
    // 如果在同一队列，还需要检查队列本地索引
    if (primary_queue == other.primary_queue && level_conflict)
    {
        return !(end_queue_local_index < other.start_queue_local_index ||
                start_queue_local_index > other.end_queue_local_index);
    }
    
    return level_conflict;
}

ResourceLifetimeAnalysis::ResourceLifetimeAnalysis(
    const PassDependencyAnalysis& dependency_analysis,
    const QueueSchedule& queue_schedule,
    const ResourceLifetimeConfig& config)
    : config_(config)
    , dependency_analysis_(dependency_analysis)
    , queue_schedule_(queue_schedule)
{
}

void ResourceLifetimeAnalysis::on_initialize(RenderGraph* graph) SKR_NOEXCEPT
{
    lifetime_result_.resource_lifetimes.reserve(128);
    lifetime_result_.resources_by_size_desc.reserve(128);
    lifetime_result_.cross_queue_resources.reserve(32);
}

void ResourceLifetimeAnalysis::on_finalize(RenderGraph* graph) SKR_NOEXCEPT
{
    lifetime_result_.resource_lifetimes.clear();
    lifetime_result_.resources_by_size_desc.clear();
    lifetime_result_.cross_queue_resources.clear();
}

void ResourceLifetimeAnalysis::on_execute(RenderGraph* graph, RenderGraphProfiler* profiler) SKR_NOEXCEPT
{
    SKR_LOG_INFO(u8"ResourceLifetimeAnalysis: Starting resource lifetime analysis");
    
    // 清理之前的结果
    lifetime_result_.resource_lifetimes.clear();
    lifetime_result_.resources_by_size_desc.clear();
    lifetime_result_.cross_queue_resources.clear();
    
    // 核心分析流程
    analyze_resource_lifetimes(graph);
    
    if (config_.use_dependency_levels)
    {
        calculate_dependency_level_lifetimes();
    }
    
    calculate_queue_local_lifetimes();
    
    if (config_.track_cross_queue_usage)
    {
        identify_cross_queue_resources();
    }
    
    if (config_.calculate_memory_stats)
    {
        calculate_memory_requirements();
        sort_resources_by_size();
        collect_lifetime_statistics();
    }
    
    if (config_.enable_debug_output)
    {
        dump_lifetimes();
        dump_aliasing_candidates();
    }
    
    SKR_LOG_INFO(u8"ResourceLifetimeAnalysis: Analyzed {} resources, {} cross-queue", 
                lifetime_result_.resource_lifetimes.size(),
                lifetime_result_.cross_queue_resources.size());
}

void ResourceLifetimeAnalysis::analyze_resource_lifetimes(RenderGraph* graph) SKR_NOEXCEPT
{
    auto& all_passes = get_passes(graph);
    auto& all_resources = get_resources(graph);
    const auto& schedule_result = queue_schedule_.get_schedule_result();
    
    // 为每个资源分析生命周期
    for (auto* resource : all_resources)
    {
        ResourceLifetime& lifetime = lifetime_result_.resource_lifetimes[resource];
        lifetime.resource = resource;
        lifetime.start_dependency_level = UINT32_MAX;
        lifetime.end_dependency_level = 0;
        lifetime.start_queue_local_index = UINT32_MAX;
        lifetime.end_queue_local_index = 0;
        
        // 收集使用该资源的所有Pass
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
                lifetime.using_passes.add(pass);
                
                // 更新依赖级别范围
                uint32_t pass_level = dependency_analysis_.get_logical_dependency_level(pass);
                lifetime.start_dependency_level = std::min(lifetime.start_dependency_level, pass_level);
                lifetime.end_dependency_level = std::max(lifetime.end_dependency_level, pass_level);
            }
        }
        
        // 处理没有使用者的资源
        if (lifetime.using_passes.is_empty())
        {
            lifetime.start_dependency_level = 0;
            lifetime.end_dependency_level = 0;
        }
    }
}

void ResourceLifetimeAnalysis::calculate_dependency_level_lifetimes() SKR_NOEXCEPT
{
    // 基于博客建议：使用dependency level indices支持并行执行
    // 这是算法的核心创新点
    
    for (auto& [resource, lifetime] : lifetime_result_.resource_lifetimes)
    {
        if (lifetime.using_passes.is_empty()) continue;
        
        // 已经在analyze_resource_lifetimes中计算了依赖级别
        // 这里可以进行进一步的优化或验证
        
        if (config_.enable_debug_output)
        {
            SKR_LOG_TRACE(u8"Resource %s: dependency levels [%u, %u]",
                         resource->get_name(),
                         lifetime.start_dependency_level,
                         lifetime.end_dependency_level);
        }
    }
}

void ResourceLifetimeAnalysis::calculate_queue_local_lifetimes() SKR_NOEXCEPT
{
    const auto& schedule_result = queue_schedule_.get_schedule_result();
    
    for (auto& [resource, lifetime] : lifetime_result_.resource_lifetimes)
    {
        if (lifetime.using_passes.is_empty()) continue;
        
        // 统计每个队列中的使用情况
        skr::FlatHashMap<uint32_t, skr::Vector<uint32_t>> queue_indices;
        
        for (auto* pass : lifetime.using_passes)
        {
            auto queue_it = schedule_result.pass_queue_assignments.find(pass);
            if (queue_it != schedule_result.pass_queue_assignments.end())
            {
                uint32_t queue_index = queue_it->second;
                
                // 找到该Pass在队列中的本地索引
                const auto& queue_schedule = schedule_result.queue_schedules[queue_index];
                for (size_t i = 0; i < queue_schedule.size(); ++i)
                {
                    if (queue_schedule[i] == pass)
                    {
                        queue_indices[queue_index].add(static_cast<uint32_t>(i));
                        break;
                    }
                }
            }
        }
        
        // 确定主要使用的队列（使用次数最多的队列）
        uint32_t max_usage = 0;
        for (const auto& [queue_idx, indices] : queue_indices)
        {
            if (indices.size() > max_usage)
            {
                max_usage = static_cast<uint32_t>(indices.size());
                lifetime.primary_queue = queue_idx;
                
                // 计算在主队列中的本地索引范围
                lifetime.start_queue_local_index = *std::min_element(indices.begin(), indices.end());
                lifetime.end_queue_local_index = *std::max_element(indices.begin(), indices.end());
            }
        }
        
        // 检查是否跨队列使用
        lifetime.is_cross_queue = queue_indices.size() > 1;
    }
}

void ResourceLifetimeAnalysis::identify_cross_queue_resources() SKR_NOEXCEPT
{
    for (const auto& [resource, lifetime] : lifetime_result_.resource_lifetimes)
    {
        if (lifetime.is_cross_queue)
        {
            lifetime_result_.cross_queue_resources.add(resource);
        }
    }
    
    SKR_LOG_INFO(u8"ResourceLifetimeAnalysis: Found {} cross-queue resources",
                lifetime_result_.cross_queue_resources.size());
}

void ResourceLifetimeAnalysis::calculate_memory_requirements() SKR_NOEXCEPT
{
    lifetime_result_.total_memory_requirement = 0;
    
    for (auto& [resource, lifetime] : lifetime_result_.resource_lifetimes)
    {
        lifetime.memory_size = estimate_resource_memory_size(resource);
        lifetime.memory_alignment = get_resource_memory_alignment(resource);
        
        lifetime_result_.total_memory_requirement += lifetime.memory_size;
    }
}

void ResourceLifetimeAnalysis::sort_resources_by_size() SKR_NOEXCEPT
{
    // 博客算法核心：按大小降序排列资源，用于内存别名化
    lifetime_result_.resources_by_size_desc.clear();
    lifetime_result_.resources_by_size_desc.reserve(lifetime_result_.resource_lifetimes.size());
    
    for (const auto& [resource, lifetime] : lifetime_result_.resource_lifetimes)
    {
        lifetime_result_.resources_by_size_desc.add(resource);
    }
    
    // 按内存大小降序排序（最大的在前面）
    std::sort(lifetime_result_.resources_by_size_desc.begin(), 
              lifetime_result_.resources_by_size_desc.end(),
              [this](ResourceNode* a, ResourceNode* b) {
                  const auto& lifetime_a = lifetime_result_.resource_lifetimes[a];
                  const auto& lifetime_b = lifetime_result_.resource_lifetimes[b];
                  return lifetime_a.memory_size > lifetime_b.memory_size;
              });
    
    if (config_.enable_debug_output)
    {
        SKR_LOG_INFO(u8"ResourceLifetimeAnalysis: Resources sorted by size (top 5):");
        for (size_t i = 0; i < std::min(size_t(5), lifetime_result_.resources_by_size_desc.size()); ++i)
        {
            auto* resource = lifetime_result_.resources_by_size_desc[i];
            const auto& lifetime = lifetime_result_.resource_lifetimes[resource];
            SKR_LOG_INFO(u8"  %zu. %s: %llu bytes", i + 1, resource->get_name(), lifetime.memory_size);
        }
    }
}

void ResourceLifetimeAnalysis::collect_lifetime_statistics() SKR_NOEXCEPT
{
    lifetime_result_.total_resource_count = static_cast<uint32_t>(lifetime_result_.resource_lifetimes.size());
    
    // 计算在任意时刻的最大并发资源数
    uint32_t max_level = 0;
    for (const auto& [resource, lifetime] : lifetime_result_.resource_lifetimes)
    {
        max_level = std::max(max_level, lifetime.end_dependency_level);
    }
    
    lifetime_result_.max_concurrent_resources = 0;
    for (uint32_t level = 0; level <= max_level; ++level)
    {
        uint32_t concurrent_count = 0;
        for (const auto& [resource, lifetime] : lifetime_result_.resource_lifetimes)
        {
            if (level >= lifetime.start_dependency_level && level <= lifetime.end_dependency_level)
            {
                concurrent_count++;
            }
        }
        lifetime_result_.max_concurrent_resources = std::max(lifetime_result_.max_concurrent_resources, concurrent_count);
    }
}

uint64_t ResourceLifetimeAnalysis::estimate_resource_memory_size(ResourceNode* resource) const SKR_NOEXCEPT
{
    // 简化的内存大小估算，实际应该基于资源类型和格式
    if (resource->get_type() == EObjectType::Texture)
    {
        // 假设是1920x1080的RGBA8纹理
        return 1920 * 1080 * 4;
    }
    else if (resource->get_type() == EObjectType::Buffer)
    {
        // 从资源描述中获取缓冲区大小
        auto* buffer_node = static_cast<const BufferNode*>(resource);
        return buffer_node->get_desc().size;
    }
    
    return 4096; // 默认大小
}

uint32_t ResourceLifetimeAnalysis::get_resource_memory_alignment(ResourceNode* resource) const SKR_NOEXCEPT
{
    // GPU资源通常需要特定的对齐
    if (resource->get_type() == EObjectType::Texture)
    {
        return 256; // 纹理对齐
    }
    else if (resource->get_type() == EObjectType::Buffer)
    {
        return 64; // 缓冲区对齐
    }
    
    return 16; // 默认对齐
}

const ResourceLifetime* ResourceLifetimeAnalysis::get_resource_lifetime(ResourceNode* resource) const
{
    auto it = lifetime_result_.resource_lifetimes.find(resource);
    return (it != lifetime_result_.resource_lifetimes.end()) ? &it->second : nullptr;
}

bool ResourceLifetimeAnalysis::resources_can_alias(ResourceNode* res1, ResourceNode* res2) const
{
    const auto* lifetime1 = get_resource_lifetime(res1);
    const auto* lifetime2 = get_resource_lifetime(res2);
    
    if (!lifetime1 || !lifetime2) return false;
    
    // 不能与自己别名
    if (res1 == res2) return false;
    
    // 检查生命周期冲突
    return !lifetime1->conflicts_with(*lifetime2);
}

void ResourceLifetimeAnalysis::dump_lifetimes() const SKR_NOEXCEPT
{
    SKR_LOG_INFO(u8"========== Resource Lifetime Analysis Results ==========");
    SKR_LOG_INFO(u8"Total resources: %u", lifetime_result_.total_resource_count);
    SKR_LOG_INFO(u8"Cross-queue resources: %zu", lifetime_result_.cross_queue_resources.size());
    SKR_LOG_INFO(u8"Total memory requirement: %llu MB", lifetime_result_.total_memory_requirement / (1024 * 1024));
    SKR_LOG_INFO(u8"Max concurrent resources: %u", lifetime_result_.max_concurrent_resources);
    
    // 显示部分资源的详细信息
    size_t count = 0;
    for (const auto& [resource, lifetime] : lifetime_result_.resource_lifetimes)
    {
        if (count++ >= 10) break; // 只显示前10个
        
        SKR_LOG_INFO(u8"Resource: %s", resource->get_name());
        SKR_LOG_INFO(u8"  Dependency levels: [%u, %u] (span: %u)",
                    lifetime.start_dependency_level,
                    lifetime.end_dependency_level,
                    lifetime.get_lifetime_span());
        SKR_LOG_INFO(u8"  Queue local indices: [%u, %u] on queue %u",
                    lifetime.start_queue_local_index,
                    lifetime.end_queue_local_index,
                    lifetime.primary_queue);
        SKR_LOG_INFO(u8"  Memory: %llu bytes, alignment: %u",
                    lifetime.memory_size,
                    lifetime.memory_alignment);
        SKR_LOG_INFO(u8"  Cross-queue: %s, using passes: %zu",
                    lifetime.is_cross_queue ? "Yes" : "No",
                    lifetime.using_passes.size());
    }
    
    SKR_LOG_INFO(u8"=======================================================");
}

void ResourceLifetimeAnalysis::dump_aliasing_candidates() const SKR_NOEXCEPT
{
    SKR_LOG_INFO(u8"========== Memory Aliasing Candidates ==========");
    
    if (lifetime_result_.resources_by_size_desc.size() < 2)
    {
        SKR_LOG_INFO(u8"Not enough resources for aliasing analysis");
        return;
    }
    
    // 检查前几个最大的资源之间的别名可能性
    for (size_t i = 0; i < std::min(size_t(5), lifetime_result_.resources_by_size_desc.size()); ++i)
    {
        auto* res1 = lifetime_result_.resources_by_size_desc[i];
        const auto& lifetime1 = lifetime_result_.resource_lifetimes.at(res1);
        
        SKR_LOG_INFO(u8"Resource %s (%llu bytes) can alias with:",
                    res1->get_name(), lifetime1.memory_size);
        
        size_t alias_count = 0;
        for (size_t j = i + 1; j < lifetime_result_.resources_by_size_desc.size(); ++j)
        {
            auto* res2 = lifetime_result_.resources_by_size_desc[j];
            
            if (resources_can_alias(res1, res2))
            {
                const auto& lifetime2 = lifetime_result_.resource_lifetimes.at(res2);
                SKR_LOG_INFO(u8"  - %s (%llu bytes)", res2->get_name(), lifetime2.memory_size);
                alias_count++;
                
                if (alias_count >= 3) break; // 最多显示3个别名候选
            }
        }
        
        if (alias_count == 0)
        {
            SKR_LOG_INFO(u8"  - No aliasing candidates found");
        }
    }
    
    SKR_LOG_INFO(u8"===============================================");
}

} // namespace render_graph
} // namespace skr