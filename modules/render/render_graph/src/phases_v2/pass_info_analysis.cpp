#include "SkrCore/log.hpp"
#include "SkrContainersDef/set.hpp"
#include "SkrRenderGraph/phases_v2/pass_info_analysis.hpp"
#include "SkrRenderGraph/frontend/pass_node.hpp"
#include "SkrRenderGraph/frontend/resource_node.hpp"
#include "SkrRenderGraph/frontend/render_graph.hpp"
#include "SkrGraphics/flags.h"

namespace skr {
namespace render_graph {

void PassInfoAnalysis::on_initialize(RenderGraph* graph) SKR_NOEXCEPT
{
    pass_infos.reserve(128);
}

void PassInfoAnalysis::on_execute(RenderGraph* graph, RenderGraphProfiler* profiler) SKR_NOEXCEPT
{
    pass_infos.clear();
    global_state_analysis_.resource_rerouting_info.clear();
    global_state_analysis_.total_rerouting_required = 0;
    
    auto& passes = get_passes(graph);
    for (PassNode* pass : passes)
    {
        extract_pass_info(pass);
    }
    
    // 分析资源状态使用模式
    analyze_resource_state_usage(graph);
}

void PassInfoAnalysis::on_finalize(RenderGraph* graph) SKR_NOEXCEPT
{
    pass_infos.clear();
    global_state_analysis_.resource_rerouting_info.clear();
}

const PassInfo* PassInfoAnalysis::get_pass_info(PassNode* pass) const
{
    auto it = pass_infos.find(pass);
    return (it != pass_infos.end()) ? &it->second : nullptr;
}

const PassResourceInfo* PassInfoAnalysis::get_resource_info(PassNode* pass) const
{
    const auto* info = get_pass_info(pass);
    return info ? &info->resource_info : nullptr;
}

const PassPerformanceInfo* PassInfoAnalysis::get_performance_info(PassNode* pass) const
{
    const auto* info = get_pass_info(pass);
    return info ? &info->performance_info : nullptr;
}

void PassInfoAnalysis::extract_pass_info(PassNode* pass)
{
    PassInfo info;
    info.pass = pass;
    info.pass_type = pass->pass_type;
    
    extract_resource_info(pass, info.resource_info);
    extract_performance_info(pass, info.performance_info);
    
    pass_infos[pass] = info;
}

void PassInfoAnalysis::extract_resource_info(PassNode* pass, PassResourceInfo& info)
{
    info.read_resources.clear();
    info.write_resources.clear();
    info.readwrite_resources.clear();
    info.all_resource_accesses.clear();
    info.has_graphics_resources = false;
    
    // Extract textures with detailed access info
    pass->foreach_textures([&](TextureNode* texture, TextureEdge* edge) {
        if (!texture) return true;
        
        ResourceAccessInfo access_info;
        access_info.resource = texture;
        
        switch (edge->get_type())
        {
        case ERelationshipType::TextureRead:
            info.read_resources.add(texture);
            access_info.access_type = EResourceAccessType::Read;
            access_info.resource_state = static_cast<TextureReadEdge*>(edge)->requested_state;
            break;
        case ERelationshipType::TextureWrite:
            info.write_resources.add(texture);
            access_info.access_type = EResourceAccessType::Write;
            access_info.resource_state = static_cast<TextureRenderEdge*>(edge)->requested_state;
            break;
        case ERelationshipType::TextureReadWrite:
            info.readwrite_resources.add(texture);
            access_info.access_type = EResourceAccessType::ReadWrite;
            access_info.resource_state = static_cast<TextureReadWriteEdge*>(edge)->requested_state;
            break;
        default:
            break;
        }
        
        info.all_resource_accesses.add(access_info);
        
        // Check for graphics resources - only writing to render targets forces graphics queue
        const auto& desc = texture->get_desc();
        if ((desc.descriptors & (CGPU_RESOURCE_TYPE_RENDER_TARGET | CGPU_RESOURCE_TYPE_DEPTH_STENCIL)) &&
            (access_info.access_type == EResourceAccessType::Write || access_info.access_type == EResourceAccessType::ReadWrite))
        {
            info.has_graphics_resources = true;
        }
        
        // Simple size calculation
        size_t size = desc.width * desc.height * desc.depth * 4; // Assume 4 bytes per pixel
        
        return true;
    });
    
    // Extract buffers with detailed access info
    pass->foreach_buffers([&](BufferNode* buffer, BufferEdge* edge) {
        if (!buffer) return true;
        
        ResourceAccessInfo access_info;
        access_info.resource = buffer;
        access_info.resource_state = static_cast<BufferEdge*>(edge)->requested_state;
        
        switch (edge->get_type())
        {
        case ERelationshipType::BufferRead:
            info.read_resources.add(buffer);
            access_info.access_type = EResourceAccessType::Read;
            break;
        case ERelationshipType::BufferReadWrite:
            info.readwrite_resources.add(buffer);
            access_info.access_type = EResourceAccessType::ReadWrite;
            break;
        default:
            break;
        }
        
        info.all_resource_accesses.add(access_info);
        return true;
    });
    
    info.total_resource_count = info.read_resources.size() + 
                               info.write_resources.size() + 
                               info.readwrite_resources.size();
}

void PassInfoAnalysis::extract_performance_info(PassNode* pass, PassPerformanceInfo& info)
{
    // Direct flag reading only
    info.is_compute_intensive = pass->has_flags(EPassFlags::ComputeIntensive);
    info.is_bandwidth_intensive = pass->has_flags(EPassFlags::BandwidthIntensive);
    info.is_vertex_bound = pass->has_flags(EPassFlags::VertexBoundIntensive);
    info.is_pixel_bound = pass->has_flags(EPassFlags::PixelBoundIntensive);
    info.has_small_working_set = pass->has_flags(EPassFlags::SmallWorkingSet);
    info.has_large_working_set = pass->has_flags(EPassFlags::LargeWorkingSet);
    info.has_random_access = pass->has_flags(EPassFlags::RandomAccess);
    info.has_streaming_access = pass->has_flags(EPassFlags::StreamingAccess);
    info.prefers_async_compute = pass->has_flags(EPassFlags::PreferAsyncCompute);
    info.separate_command_buffer = pass->has_flags(EPassFlags::SeperateFromCommandBuffer);
}

// For dependency analysis - avoid recomputation
EResourceAccessType PassInfoAnalysis::get_resource_access_type(PassNode* pass, ResourceNode* resource) const
{
    const auto* resource_info = get_resource_info(pass);
    if (!resource_info) return EResourceAccessType::Read;
    
    // Quick lookup in pre-computed access info
    for (const auto& access : resource_info->all_resource_accesses)
    {
        if (access.resource == resource)
        {
            return access.access_type;
        }
    }
    
    return EResourceAccessType::Read; // Default fallback
}

ECGPUResourceState PassInfoAnalysis::get_resource_state(PassNode* pass, ResourceNode* resource) const
{
    const auto* resource_info = get_resource_info(pass);
    if (!resource_info) return CGPU_RESOURCE_STATE_UNDEFINED;
    
    // Quick lookup in pre-computed access info
    for (const auto& access : resource_info->all_resource_accesses)
    {
        if (access.resource == resource)
        {
            return access.resource_state;
        }
    }
    
    return CGPU_RESOURCE_STATE_UNDEFINED; // Default fallback
}

const ResourceReroutingInfo* PassInfoAnalysis::get_resource_rerouting_info(ResourceNode* resource) const
{
    auto it = global_state_analysis_.resource_rerouting_info.find(resource);
    return (it != global_state_analysis_.resource_rerouting_info.end()) ? &it->second : nullptr;
}

bool PassInfoAnalysis::resource_needs_rerouting(ResourceNode* resource) const
{
    const auto* info = get_resource_rerouting_info(resource);
    return info ? info->needs_rerouting : false;
}

bool PassInfoAnalysis::resource_requires_graphics_queue(ResourceNode* resource) const
{
    const auto* info = get_resource_rerouting_info(resource);
    return info ? info->requires_graphics_queue : false;
}

bool PassInfoAnalysis::resource_requires_compute_queue(ResourceNode* resource) const
{
    const auto* info = get_resource_rerouting_info(resource);
    return info ? info->requires_compute_queue : false;
}

static const skr::Set<ECGPUResourceState> graphics_only_set = {
    CGPU_RESOURCE_STATE_RENDER_TARGET,
    CGPU_RESOURCE_STATE_DEPTH_WRITE,
    CGPU_RESOURCE_STATE_DEPTH_READ,
    CGPU_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
    CGPU_RESOURCE_STATE_INDEX_BUFFER,
    CGPU_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
};

static const skr::Set<ECGPUResourceState> compute_compatible_set = {
    CGPU_RESOURCE_STATE_UNORDERED_ACCESS,
    CGPU_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
    CGPU_RESOURCE_STATE_COPY_SOURCE,
    CGPU_RESOURCE_STATE_COPY_DEST
};

void PassInfoAnalysis::analyze_resource_state_usage(RenderGraph* graph)
{
    // 检查图形和计算队列兼容的状态
    const ECGPUResourceState graphics_only_states[] = {
        CGPU_RESOURCE_STATE_RENDER_TARGET,
        CGPU_RESOURCE_STATE_DEPTH_WRITE,
        CGPU_RESOURCE_STATE_DEPTH_READ,
        CGPU_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
        CGPU_RESOURCE_STATE_INDEX_BUFFER,
        CGPU_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    };
    
    const ECGPUResourceState compute_compatible_states[] = {
        CGPU_RESOURCE_STATE_UNORDERED_ACCESS,
        CGPU_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        CGPU_RESOURCE_STATE_COPY_SOURCE,
        CGPU_RESOURCE_STATE_COPY_DEST
    };
    
    // 收集所有资源的使用状态
    skr::FlatHashMap<ResourceNode*, skr::FlatHashSet<ECGPUResourceState>> resource_states_map;
    
    for (const auto& pass_pair : pass_infos)
    {
        const auto& resource_info = pass_pair.second.resource_info;
        
        for (const auto& access : resource_info.all_resource_accesses)
        {
            ResourceNode* resource = access.resource;
            ECGPUResourceState state = access.resource_state;
            
            // 确保资源在 map 中存在
            if (resource_states_map.find(resource) == resource_states_map.end())
            {
                resource_states_map[resource] = skr::FlatHashSet<ECGPUResourceState>();
            }
            
            resource_states_map[resource].insert(state);
        }
    }
    
    // 分析每个资源的队列要求
    for (const auto& pair : resource_states_map)
    {
        ResourceNode* resource = pair.first;
        const auto& states = pair.second;
        
        ResourceReroutingInfo rerouting_info;
        rerouting_info.resource = resource;
        rerouting_info.all_used_states = states;
        
        // 判断是否需要图形队列
        for (ECGPUResourceState state : states)
        {
            if (graphics_only_set.contains(state))
            {
                rerouting_info.requires_graphics_queue = true;
                break;
            }
        }
        
        // 判断是否可以用计算队列
        bool all_compute_compatible = true;
        for (ECGPUResourceState state : states)
        {
            if (!compute_compatible_set.contains(state))
            {
                all_compute_compatible = false;
                break;
            }
        }
        rerouting_info.requires_compute_queue = all_compute_compatible;
        
        // 判断是否需要重路由（同时需要图形和计算队列的情况）
        rerouting_info.needs_rerouting = rerouting_info.requires_graphics_queue && rerouting_info.requires_compute_queue;
        
        global_state_analysis_.resource_rerouting_info[resource] = rerouting_info;
        
        if (rerouting_info.needs_rerouting)
        {
            global_state_analysis_.total_rerouting_required++;
        }
    }
}

} // namespace render_graph
} // namespace skr