#include "SkrRenderGraph/phases_v2/pass_info_analysis.hpp"
#include "SkrRenderGraph/frontend/pass_node.hpp"
#include "SkrRenderGraph/frontend/resource_node.hpp"
#include "SkrCore/log.hpp"

namespace skr {
namespace render_graph {

void PassInfoAnalysis::on_initialize(RenderGraph* graph) SKR_NOEXCEPT
{
    pass_infos.reserve(128);
}

void PassInfoAnalysis::on_execute(RenderGraph* graph, RenderGraphProfiler* profiler) SKR_NOEXCEPT
{
    pass_infos.clear();
    
    auto& passes = get_passes(graph);
    for (PassNode* pass : passes)
    {
        extract_pass_info(pass);
    }
}

void PassInfoAnalysis::on_finalize(RenderGraph* graph) SKR_NOEXCEPT
{
    pass_infos.clear();
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

} // namespace render_graph
} // namespace skr