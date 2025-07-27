#include "SkrGraphics/api.h"
#include "SkrGraphics/flags.h"
#include "SkrProfile/profile.h"
#include "SkrBase/misc/make_zeroed.hpp"
#include "SkrCore/time.h"
#include "SkrCore/platform/vfs.h"
#include "SkrRT/ecs/query.hpp"
#include "SkrRenderer/render_device.h"

#include "scene_renderer.hpp"
#include "helper.hpp"

struct SceneRendererImpl : public skr::SceneRenderer
{
    const char8_t* push_constants_name = u8"push_constants";
    const ECGPUFormat color_format = CGPU_FORMAT_B8G8R8A8_UNORM;
    const ECGPUFormat depth_format = CGPU_FORMAT_D32_SFLOAT;

    skr_vfs_t* resource_vfs;

    uint64_t frame_count;

    CGPURenderPipelineId pipeline = nullptr;
    CGPUVertexLayout vertex_layout = {};
    CGPURasterizerStateDescriptor rs_state = {};
    CGPUDepthStateDescriptor depth_state = {};

    // resources
    CGPUBufferId vertex_buffer = nullptr;
    CGPUBufferId index_buffer = nullptr;
    CGPUBufferId instance_buffer = nullptr;

    void initialize(skr::RendererDevice* render_device, skr::ecs::World* world, struct skr_vfs_t* resource_vfs) override
    {
        this->resource_vfs = resource_vfs;
        // TODO: effect_query

        prepare_pipeline_settings();
        prepare_pipeline(render_device);
        // TODO: reset_render_view
    }

    void finalize(skr::RendererDevice* renderer) override
    {
        free_pipeline(renderer);
    }

    void draw(skr::render_graph::RenderGraph* render_graph) override
    {
        // TODO: create_frame_resources
        // TODO: push drawcalls to rendergraph
    }

    void produce_drawcalls(sugoi_storage_t* storage, skr::render_graph::RenderGraph* render_graph) override
    {
        frame_count++;
        produce_mesh_drawcall(storage, render_graph);
    }

    void produce_mesh_drawcall(sugoi_storage_t* storage, skr::render_graph::RenderGraph* render_graph)
    {
        // TODO: Camera -> View Matrix
        // TODO: Query Storage to iterate render model and produce drawcalls
    }

    // helpers
    uint32_t* read_shader_bytes(skr::RendererDevice* render_device, const char8_t* name, uint32_t* out_length);
    CGPUShaderLibraryId create_shader_library(skr::RendererDevice* render_device, const char8_t* name, ECGPUShaderStage stage);

    void prepare_pipeline_settings();
    void prepare_pipeline(skr::RendererDevice* render_device);
    void free_pipeline(skr::RendererDevice* renderer);
};

// SceneRendererImpl* scene_effect = SkrNew<SceneRendererImpl>();
skr::SceneRenderer* skr::SceneRenderer::Create()
{
    return SkrNew<SceneRendererImpl>();
}

void skr::SceneRenderer::Destroy(skr::SceneRenderer* renderer)
{
    SkrDelete(renderer);
}

skr::SceneRenderer::~SceneRenderer() {}

void SceneRendererImpl::prepare_pipeline_settings()
{
    // max vertex attributes is 15
    vertex_layout.attributes[0] = { u8"position", 1, CGPU_FORMAT_R32G32B32_SFLOAT, 0, 0, sizeof(skr_float3_t), CGPU_INPUT_RATE_VERTEX };     // per vertex position
    vertex_layout.attributes[1] = { u8"color", 1, CGPU_FORMAT_R32G32B32_SFLOAT, 1, 0, sizeof(skr_float3_t), CGPU_INPUT_RATE_VERTEX };        // per vertex color
    vertex_layout.attributes[2] = { u8"model", 4, CGPU_FORMAT_R32G32B32A32_SFLOAT, 3, 0, sizeof(skr_float4x4_t), CGPU_INPUT_RATE_INSTANCE }; // per-instance model matrix
    vertex_layout.attribute_count = 3;

    // rasterize state
    rs_state.cull_mode = CGPU_CULL_MODE_BACK;
    rs_state.depth_bias = 0;
    rs_state.fill_mode = CGPU_FILL_MODE_SOLID;
    rs_state.front_face = CGPU_FRONT_FACE_CCW;

    depth_state.depth_write = true;
    depth_state.depth_test = true;
}

void SceneRendererImpl::prepare_pipeline(skr::RendererDevice* render_device)
{
    const auto cgpu_device = render_device->get_cgpu_device();
    CGPUShaderLibraryId vs = utils::create_shader_library(render_device, resource_vfs, u8"shaders/scene/debug.vs", CGPU_SHADER_STAGE_VERT);
    CGPUShaderLibraryId fs = utils::create_shader_library(render_device, resource_vfs, u8"shaders/scene/debug.fs", CGPU_SHADER_STAGE_FRAG);

    CGPUShaderEntryDescriptor ppl_shaders[2];
    CGPUShaderEntryDescriptor& ppl_vs = ppl_shaders[0];
    ppl_vs.library = vs;
    ppl_vs.stage = CGPU_SHADER_STAGE_VERT;
    ppl_vs.entry = u8"vs";
    CGPUShaderEntryDescriptor& ppl_fs = ppl_shaders[1];
    ppl_fs.library = fs;
    ppl_fs.stage = CGPU_SHADER_STAGE_FRAG;
    ppl_fs.entry = u8"fs";

    auto rs_desc = make_zeroed<CGPURootSignatureDescriptor>();
    rs_desc.push_constant_count = 1;
    rs_desc.push_constant_names = &push_constants_name;
    rs_desc.shader_count = 2;
    rs_desc.shaders = ppl_shaders;
    rs_desc.pool = render_device->get_root_signature_pool();
    auto root_sig = cgpu_create_root_signature(cgpu_device, &rs_desc);

    CGPURenderPipelineDescriptor rp_desc = {};
    rp_desc.root_signature = root_sig;
    rp_desc.prim_topology = CGPU_PRIM_TOPO_TRI_LIST;
    rp_desc.vertex_layout = &vertex_layout;
    rp_desc.vertex_shader = &ppl_vs;
    rp_desc.fragment_shader = &ppl_fs;
    rp_desc.render_target_count = 1;
    rp_desc.color_formats = &color_format;
    rp_desc.depth_stencil_format = depth_format;

    rp_desc.rasterizer_state = &rs_state;
    pipeline = cgpu_create_render_pipeline(cgpu_device, &rp_desc);
    cgpu_free_shader_library(fs);
    cgpu_free_shader_library(vs);
}

void SceneRendererImpl::free_pipeline(skr::RendererDevice* renderer)
{
    auto sig_to_free = pipeline->root_signature;
    cgpu_free_render_pipeline(pipeline);
    cgpu_free_root_signature(sig_to_free);
}

// render_view_reset
// render_view_set_screen
// render_view_set_transform_screen
// render_view_transform_view