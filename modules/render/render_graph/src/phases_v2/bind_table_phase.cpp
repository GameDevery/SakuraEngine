#include "SkrRenderGraph/phases_v2/bind_table_phase.hpp"
#include "SkrRenderGraph/frontend/pass_node.hpp"
#include "SkrRenderGraph/frontend/resource_node.hpp"
#include "SkrRenderGraph/frontend/render_graph.hpp"
#include "SkrCore/log.hpp"
#include "SkrProfile/profile.h"
#include "SkrGraphics/flags.h"

#define BIND_TABLE_LOG SKR_LOG_DEBUG

namespace skr {
namespace render_graph {

// Utility function to find shader resource (from old implementation)
const CGPUShaderResource* find_shader_resource(uint64_t name_hash, CGPURootSignatureId root_sig, ECGPUResourceType* type = nullptr)
{
    for (uint32_t i = 0; i < root_sig->table_count; i++)
    {
        for (uint32_t j = 0; j < root_sig->tables[i].resources_count; j++)
        {
            const auto& resource = root_sig->tables[i].resources[j];
            if (resource.name_hash == name_hash && 
                strcmp((const char*)resource.name, (const char*)root_sig->tables[i].resources[j].name) == 0)
            {
                if (type)
                    *type = resource.type;
                return &root_sig->tables[i].resources[j];
            }
        }
    }
    return nullptr;
}

BindTablePhase::BindTablePhase(
    const PassInfoAnalysis& pass_info_analysis,
    const ResourceAllocationPhase& resource_allocation_phase)
    : pass_info_analysis_(pass_info_analysis)
    , resource_allocation_phase_(resource_allocation_phase)
{
}

void BindTablePhase::on_initialize(RenderGraph* graph) SKR_NOEXCEPT
{
    auto backend = static_cast<RenderGraphBackend*>(graph);
    bind_table_result_.pass_bind_tables.reserve(256);
}

void BindTablePhase::on_finalize(RenderGraph* graph) SKR_NOEXCEPT
{
    bind_table_result_.pass_bind_tables.clear();
}

void BindTablePhase::on_execute(RenderGraph* graph, RenderGraphFrameExecutor* executor, RenderGraphProfiler* profiler) SKR_NOEXCEPT
{
    SkrZoneScopedN("BindTablePhase");
    BIND_TABLE_LOG(u8"BindTablePhase: Starting bind table creation");
    
    // Clear previous results
    bind_table_result_.pass_bind_tables.clear();
    bind_table_result_.total_bind_tables_created = 0;
    bind_table_result_.total_texture_views_created = 0;
    
    // Get all passes from graph
    const auto& all_passes = get_passes(graph);
    
    for (auto* pass : all_passes)
    {
        // Get root signature based on pass type
        CGPURootSignatureId root_sig = nullptr;
        if (pass->pass_type == EPassType::Render)
        {
            auto* render_pass = static_cast<RenderPassNode*>(pass);
            root_sig = render_pass->get_root_signature();
        }
        if (pass->pass_type == EPassType::Compute)
        {
            auto* compute_pass = static_cast<ComputePassNode*>(pass);
            root_sig = compute_pass->get_root_signature();
        }
        
        // Skip passes without root signature
        if (!root_sig) continue;
        
        // Create bind table for this pass
        CGPUXBindTableId bind_table = create_bind_table_for_pass(graph, *executor, pass, root_sig);
        if (!bind_table) continue;
        
        // Store result
        PassBindTableInfo bind_info;
        bind_info.pass = pass;
        bind_info.root_signature = root_sig;
        bind_info.bind_table = bind_table;
        
        bind_table_result_.pass_bind_tables.add(pass, std::move(bind_info));
        bind_table_result_.total_bind_tables_created++;
        
        BIND_TABLE_LOG(u8"Created bind table for pass: %s", pass->get_name());
    }
    
    BIND_TABLE_LOG(u8"BindTablePhase: Created %u bind tables, %u texture views",
                  bind_table_result_.total_bind_tables_created,
                  bind_table_result_.total_texture_views_created);
}

CGPUXBindTableId BindTablePhase::create_bind_table_for_pass(RenderGraph* graph_, RenderGraphFrameExecutor& executor, PassNode* pass, CGPURootSignatureId root_sig) SKR_NOEXCEPT
{
    RenderGraphBackend* graph = static_cast<RenderGraphBackend*>(graph_);
    // Generate binding key for caching (similar to old implementation)
    skr::String bind_table_keys = u8"";
    
    // Collect all binding names and prepare descriptor updates
    skr::InlineVector<CGPUDescriptorData, 8> desc_set_updates;
    skr::InlineVector<const char8_t*, 8> bindTableValueNames;
    
    // Temporary storage for resources (released after binding)
    skr::InlineVector<CGPUBufferId, 8> cbvs;
    skr::InlineVector<CGPUTextureViewId, 8> srvs;
    skr::InlineVector<CGPUTextureViewId, 8> uavs;
    
    uint32_t texture_count = 0;
    uint32_t buffer_count = 0;
    
    // Process buffer resources (CBVs)
    auto buf_read_edges = pass->buf_read_edges();
    cbvs.resize_zeroed(buf_read_edges.size());
    
    for (uint32_t e_idx = 0; e_idx < buf_read_edges.size(); e_idx++)
    {
        auto& read_edge = buf_read_edges[e_idx];
        
        const auto& resource = *find_shader_resource(read_edge->name_hash, root_sig);
        ECGPUResourceType resource_type = resource.type;
        bind_table_keys.append(read_edge->get_name() ? read_edge->get_name() : resource.name);
        bind_table_keys.append(u8";");
        bindTableValueNames.emplace(resource.name);
        
        auto buffer_readed = read_edge->get_buffer_node();
        CGPUDescriptorData update = {};
        update.count = 1;
        update.name = resource.name;
        update.binding_type = resource_type;
        update.binding = resource.binding;
        cbvs[e_idx] = resource_allocation_phase_.get_resource(buffer_readed);
        update.buffers = &cbvs[e_idx];
        desc_set_updates.emplace(update);
        
        buffer_count++;
    }
    
    // Process texture SRV resources
    auto tex_read_edges = pass->tex_read_edges();
    srvs.resize_zeroed(tex_read_edges.size());
    
    for (uint32_t e_idx = 0; e_idx < tex_read_edges.size(); e_idx++)
    {
        auto& read_edge = tex_read_edges[e_idx];
        
        const auto& resource = *find_shader_resource(read_edge->name_hash, root_sig);
        bind_table_keys.append(read_edge->get_name() ? read_edge->get_name() : resource.name);
        bind_table_keys.append(u8";");
        bindTableValueNames.emplace(resource.name);
        
        auto texture_readed = read_edge->get_texture_node();
        CGPUDescriptorData update = {};
        update.count = 1;
        update.name = resource.name;
        update.binding_type = CGPU_RESOURCE_TYPE_TEXTURE;
        update.binding = resource.binding;
        
        // Create texture view
        CGPUTextureViewDescriptor view_desc = {};
        view_desc.texture = resource_allocation_phase_.get_resource(texture_readed);
        view_desc.base_array_layer = read_edge->get_array_base();
        view_desc.array_layer_count = read_edge->get_array_count();
        view_desc.base_mip_level = read_edge->get_mip_base();
        view_desc.mip_level_count = read_edge->get_mip_count();
        view_desc.format = view_desc.texture->info->format;
        view_desc.usages = CGPU_TVU_SRV;
        view_desc.dims = read_edge->get_dimension();
        
        const bool is_depth_stencil = FormatUtil_IsDepthStencilFormat(view_desc.format);
        const bool is_depth_only = FormatUtil_IsDepthOnlyFormat(view_desc.format);
        view_desc.aspects = is_depth_stencil ?
            (is_depth_only ? CGPU_TVA_DEPTH : CGPU_TVA_DEPTH | CGPU_TVA_STENCIL) :
            CGPU_TVA_COLOR;
        
        srvs[e_idx] = graph->get_texture_view_pool().allocate(view_desc, graph->get_frame_index());
        update.textures = &srvs[e_idx];
        desc_set_updates.emplace(update);
        
        texture_count++;
        bind_table_result_.total_texture_views_created++;
    }
    
    // Process texture UAV resources
    auto tex_rw_edges = pass->tex_readwrite_edges();
    uavs.resize_zeroed(tex_rw_edges.size());
    
    for (uint32_t e_idx = 0; e_idx < tex_rw_edges.size(); e_idx++)
    {
        auto& rw_edge = tex_rw_edges[e_idx];
        
        const auto& resource = *find_shader_resource(rw_edge->name_hash, root_sig);
        bind_table_keys.append(rw_edge->get_name() ? rw_edge->get_name() : resource.name);
        bind_table_keys.append(u8";");
        bindTableValueNames.emplace(resource.name);
        
        auto texture_readwrite = rw_edge->get_texture_node();
        CGPUDescriptorData update = {};
        update.count = 1;
        update.name = resource.name;
        update.binding_type = CGPU_RESOURCE_TYPE_RW_TEXTURE;
        update.binding = resource.binding;
        
        // Create UAV texture view
        CGPUTextureViewDescriptor view_desc = {};
        view_desc.texture = resource_allocation_phase_.get_resource(texture_readwrite);
        view_desc.base_array_layer = 0;
        view_desc.array_layer_count = 1;
        view_desc.base_mip_level = 0;
        view_desc.mip_level_count = 1;
        view_desc.aspects = CGPU_TVA_COLOR;
        view_desc.format = view_desc.texture->info->format;
        view_desc.usages = CGPU_TVU_UAV;
        view_desc.dims = CGPU_TEX_DIMENSION_2D;
        
        uavs[e_idx] = graph->get_texture_view_pool().allocate(view_desc, graph->get_frame_index());
        update.textures = &uavs[e_idx];
        desc_set_updates.emplace(update);
        
        texture_count++;
        bind_table_result_.total_texture_views_created++;
    }
    
    // Early exit if no resources to bind
    if (bindTableValueNames.is_empty()) return nullptr;
    
    // Get bind table from executor's pool (similar to old implementation)
    auto&& table_pool_iter = executor.bind_table_pools.find(root_sig);
    if (table_pool_iter == executor.bind_table_pools.end())
    {
        executor.bind_table_pools.emplace(root_sig, SkrNew<BindTablePool>(root_sig));
    }
    
    auto bind_table = executor.bind_table_pools[root_sig]->pop(bind_table_keys.c_str(), 
                                                              bindTableValueNames.data(), 
                                                              (uint32_t)bindTableValueNames.size());
    
    // Update bind table with descriptors
    cgpux_bind_table_update(bind_table, desc_set_updates.data(), (uint32_t)desc_set_updates.size());
    
    // Update statistics in result
    auto& bind_info = bind_table_result_.pass_bind_tables.try_add_default(pass).value();
    bind_info.bound_texture_count = texture_count;
    bind_info.bound_buffer_count = buffer_count;
    
    return bind_table;
}

const PassBindTableInfo* BindTablePhase::get_pass_bind_table_info(PassNode* pass) const
{
    auto it = bind_table_result_.pass_bind_tables.find(pass);
    return it ? &it.value() : nullptr;
}

CGPUXBindTableId BindTablePhase::get_pass_bind_table(PassNode* pass) const
{
    const auto* info = get_pass_bind_table_info(pass);
    return info ? info->bind_table : nullptr;
}

} // namespace render_graph
} // namespace skr