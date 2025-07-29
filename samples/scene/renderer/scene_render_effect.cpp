#include "SkrGraphics/api.h"
#include "SkrGraphics/flags.h"
#include "SkrProfile/profile.h"
#include "SkrBase/misc/make_zeroed.hpp"
#include "SkrCore/time.h"
#include "SkrCore/platform/vfs.h"
#include "SkrRT/ecs/query.hpp"
#include "SkrRenderer/primitive_draw.h"
#include "SkrRenderer/render_device.h"
#include "SkrRenderGraph/frontend/render_graph.hpp"
#include "scene_renderer.hpp"
#include "helper.hpp"

struct SceneRendererImpl : public skr::SceneRenderer
{
    const char8_t* push_constants_name = u8"push_constants";
    const ECGPUFormat color_format = CGPU_FORMAT_R8G8B8A8_UNORM;
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

    skr::Vector<skr_vertex_buffer_view_t> vbvs;
    skr_index_buffer_view_t ibv;

    skr::Vector<skr::renderer::PrimitiveCommand> primitive_commands;

    // temp mesh
    const skr_float3_t g_Positions[3] = {
        { -1.0f, -1.0f, 0.0f },
        { 1.0f, -1.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f },
    };
    const skr_float2_t g_UVs[3] = {
        { 0.0f, 1.0f },
        { 1.0f, 1.0f },
        { 0.5f, 0.0f },
    };
    const skr_float3_t g_Normals[3] = {
        { 0.0f, 0.0f, 1.0f },
        { 0.0f, 0.0f, 1.0f },
        { 0.0f, 0.0f, 1.0f },
    };
    const uint32_t g_Indices[3] = { 0, 1, 2 };
    // skr_float4x4_t g_ModelMatrix = skr_float4x4_t::identity();

    // temp push_constants
    skr_float4x4_t view_proj = skr_float4x4_t::identity();

    skr::Vector<skr_primitive_draw_t> drawcalls;
    skr_vertex_buffer_view_t vertex_buffer_view = {};

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

    void create_resource(skr::RendererDevice* renderer) override
    {
        auto cgpu_device = renderer->get_cgpu_device();
        auto vertex_size = sizeof(g_Positions) + 2 * sizeof(g_UVs) + sizeof(g_Normals);
        // auto vertex_size = sizeof(g_Positions);
        auto vb_desc = make_zeroed<CGPUBufferDescriptor>();
        vb_desc.name = u8"scene-renderer-vertices";
        vb_desc.size = vertex_size;
        vb_desc.memory_usage = CGPU_MEM_USAGE_CPU_TO_GPU;
        vb_desc.descriptors = CGPU_RESOURCE_TYPE_VERTEX_BUFFER;
        vb_desc.flags = CGPU_BCF_PERSISTENT_MAP_BIT;
        vb_desc.prefer_on_device = true;
        vertex_buffer = cgpu_create_buffer(cgpu_device, &vb_desc);
        {
            memcpy(vertex_buffer->info->cpu_mapped_address, g_Positions, sizeof(g_Positions));
            memcpy((uint8_t*)vertex_buffer->info->cpu_mapped_address + sizeof(g_Positions), g_UVs, sizeof(g_UVs));
            memcpy((uint8_t*)vertex_buffer->info->cpu_mapped_address + sizeof(g_Positions) + sizeof(g_UVs), g_UVs, sizeof(g_UVs));
            memcpy((uint8_t*)vertex_buffer->info->cpu_mapped_address + sizeof(g_Positions) + 2 * sizeof(g_UVs), g_Normals, sizeof(g_Normals));
        }

        auto index_size = sizeof(g_Indices);
        auto ib_desc = make_zeroed<CGPUBufferDescriptor>();
        ib_desc.name = u8"scene-renderer-indices";
        ib_desc.size = index_size;
        ib_desc.memory_usage = CGPU_MEM_USAGE_CPU_TO_GPU;
        ib_desc.descriptors = CGPU_RESOURCE_TYPE_INDEX_BUFFER;
        ib_desc.flags = CGPU_BCF_PERSISTENT_MAP_BIT;
        ib_desc.prefer_on_device = true;
        index_buffer = cgpu_create_buffer(cgpu_device, &ib_desc);

        {
            memcpy(index_buffer->info->cpu_mapped_address, g_Indices, sizeof(g_Indices));
        }
        // construct triangle vbvs and ibv
        skr_vertex_buffer_view_t vbv = {};
        vbv.buffer = vertex_buffer;
        vbv.stride = sizeof(skr_float3_t);
        vbv.offset = 0;
        vbvs.push_back(vbv);

        vbv.buffer = vertex_buffer;
        vbv.stride = sizeof(skr_float2_t);
        vbv.offset = sizeof(g_Positions);
        vbvs.push_back(vbv);

        vbv.buffer = vertex_buffer;
        vbv.stride = sizeof(skr_float2_t);
        vbv.offset = sizeof(g_Positions) + sizeof(g_UVs);
        vbvs.push_back(vbv);

        vbv.buffer = vertex_buffer;
        vbv.stride = sizeof(skr_float3_t);
        vbv.offset = sizeof(g_Positions) + 2 * sizeof(g_UVs);
        vbvs.push_back(vbv);

        SKR_LOG_INFO(u8"vbvs size: %d", vbvs.size());

        ibv.buffer = index_buffer;
        ibv.stride = sizeof(uint32_t);
        ibv.offset = 0;
        ibv.index_count = 3;
        ibv.first_index = 0;

        // construct primitive command
        primitive_commands.resize_zeroed(1);
        auto& primitive = primitive_commands[0];
        primitive.vbvs = { vbvs.data(), (uint32_t)vbvs.size() };
        primitive.ibv = &ibv;
    }

    void render_mesh(skr_render_mesh_id render_mesh, skr::render_graph::RenderGraph* render_graph) override
    {
        SkrZoneScopedN("SceneRenderer::render_mesh");
        if (!render_mesh)
        {
            SKR_LOG_ERROR(u8"Render Mesh is null");
            return;
        }
    }

    void produce_drawcalls(sugoi_storage_t* storage, skr::render_graph::RenderGraph* render_graph) override
    {
        produce_mesh_drawcall(storage, render_graph);
    }

    void execute(sugoi_storage_t* storage, skr::render_graph::RenderGraph* render_graph)
    {
    }

    void produce_mesh_drawcall(sugoi_storage_t* storage, skr::render_graph::RenderGraph* render_graph)
    {
        // TODO: Camera -> View Matrix
        view_proj = skr_float4x4_t(.5f, 0.0f, 0.0f, 0.0f, 0.0f, .5f, 0.0f, 0.0f, 0.0f, 0.0f, .5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
        // TODO: Query Storage to iterate render model and produce drawcalls
        // skr::renderer::PrimitiveCommand cmd;
        // TODO: update model motion
        // g_ModelMatrix = skr_float4x4_t(
        //     .5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);

        // primitive_commands -> drawcalls
        auto& cmd = primitive_commands[0];
        skr_primitive_draw_t& drawcall = drawcalls.emplace().ref();
        drawcall.pipeline = pipeline;
        drawcall.push_const_name = push_constants_name;
        drawcall.push_const = (const uint8_t*)(&view_proj);
        drawcall.vertex_buffer_count = (uint32_t)cmd.vbvs.size();
        drawcall.vertex_buffers = cmd.vbvs.data();
        drawcall.index_buffer = *cmd.ibv;

        auto backbuffer = render_graph->get_texture(u8"backbuffer");
        const auto back_desc = render_graph->resolve_descriptor(backbuffer);
        render_graph->add_render_pass(
            [this, backbuffer](skr::render_graph::RenderGraph& g, skr::render_graph::RenderPassBuilder& builder) {
                builder.set_name(u8"scene_render_pass")
                    .set_pipeline(pipeline)
                    .write(0, backbuffer, CGPU_LOAD_ACTION_CLEAR);
            },
            [this, back_desc, drawcall](skr::render_graph::RenderGraph& g, skr::render_graph::RenderPassContext& context) {
                {
                    // do render pass
                    cgpu_render_encoder_set_viewport(context.encoder, 0.0, 0.0, (float)back_desc->width, (float)back_desc->height, 0.0f, 1.0f);
                    cgpu_render_encoder_set_scissor(context.encoder, 0, 0, back_desc->width, back_desc->height);

                    CGPUBufferId vertex_buffers[16] = { 0 };
                    uint32_t strides[16] = { 0 };
                    uint32_t offsets[16] = { 0 };

                    for (uint32_t i = 0; i < drawcall.vertex_buffer_count; i++)
                    {

                        vertex_buffers[i] = drawcall.vertex_buffers[i].buffer;
                        strides[i] = drawcall.vertex_buffers[i].stride;
                        offsets[i] = drawcall.vertex_buffers[i].offset;
                    }

                    cgpu_render_encoder_bind_index_buffer(context.encoder, drawcall.index_buffer.buffer, drawcall.index_buffer.stride, drawcall.index_buffer.offset);
                    cgpu_render_encoder_bind_vertex_buffers(context.encoder, drawcall.vertex_buffer_count, vertex_buffers, strides, offsets);
                    cgpu_render_encoder_push_constants(context.encoder, drawcall.pipeline->root_signature, drawcall.push_const_name, drawcall.push_const);
                    cgpu_render_encoder_draw_indexed_instanced(context.encoder, drawcall.index_buffer.index_count, drawcall.index_buffer.first_index, 1, 0, 0); // 3 vertices, 1 instance
                }
            });
    }

    void draw(skr::render_graph::RenderGraph* render_graph) override
    {
        // auto backbuffer = render_graph->get_texture(u8"backbuffer");
        // const auto back_desc = render_graph->resolve_descriptor(backbuffer);
        // if (!drawcalls.size()) return;
        // if (drawcalls.size() == 0)
        //     return; // no models need to draw

        // CGPURootSignatureId root_signature = nullptr;
        // root_signature = drawcalls[0].pipeline->root_signature;
        // render_graph->add_render_pass(
        //     [=](skr::render_graph::RenderGraph& g, skr::render_graph::RenderPassBuilder& builder) {},
        //     [=](skr::render_graph::RenderGraph& g, skr::render_graph::RenderPassContext& pass_context) {});
    }

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
    vertex_layout.attributes[0] = { u8"position", 1, CGPU_FORMAT_R32G32B32_SFLOAT, 0, 0, sizeof(skr_float3_t), CGPU_INPUT_RATE_VERTEX }; // per vertex position
    vertex_layout.attributes[1] = { u8"uv", 2, CGPU_FORMAT_R32G32_SFLOAT, 1, 0, sizeof(skr_float2_t), CGPU_INPUT_RATE_VERTEX };
    vertex_layout.attributes[2] = { u8"normal", 3, CGPU_FORMAT_R32G32B32_SFLOAT, 2, 0, sizeof(skr_float3_t), CGPU_INPUT_RATE_VERTEX }; // per vertex normal

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