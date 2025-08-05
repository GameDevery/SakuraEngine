#include "SkrGraphics/api.h"
#include "SkrGraphics/flags.h"
#include "SkrProfile/profile.h"
#include "SkrBase/misc/make_zeroed.hpp"
#include "SkrCore/time.h"
#include "SkrCore/platform/vfs.h"
#include "SkrRT/ecs/query.hpp"
#include "SkrRenderer/primitive_draw.h"
#include "SkrRenderer/render_device.h"
#include "SkrRenderer/render_mesh.h"
#include "SkrRenderGraph/frontend/render_graph.hpp"
#include "scene_renderer.hpp"
#include "helper.hpp"

struct SceneRendererImpl : public skr::SceneRenderer
{
    const char8_t* push_constants_name = u8"push_constants";
    const ECGPUFormat color_format = CGPU_FORMAT_R8G8B8A8_UNORM;
    const ECGPUFormat depth_format = CGPU_FORMAT_D32_SFLOAT;

    skr_vfs_t* resource_vfs;
    utils::Camera* mp_camera;
    CGPURenderPipelineId pipeline = nullptr;
    CGPUVertexLayout vertex_layout = {};
    CGPURasterizerStateDescriptor rs_state = {};
    CGPUDepthStateDescriptor depth_state = {};
    // temp push_constants
    skr_float4x4_t view_proj = skr_float4x4_t::identity();

    void initialize(skr::RendererDevice* render_device, skr::ecs::World* world, struct skr_vfs_t* resource_vfs) override
    {
        this->resource_vfs = resource_vfs;
        prepare_pipeline_settings();
        prepare_pipeline(render_device);
    }

    void finalize(skr::RendererDevice* renderer) override
    {
        free_pipeline(renderer);
    }

    void set_camera(utils::Camera* camera) override
    {
        mp_camera = camera;
    }

    void draw_primitives(skr::render_graph::RenderGraph* render_graph, const skr::span<skr::renderer::PrimitiveCommand> cmds) override
    {
        if (!mp_camera)
        {
            view_proj = skr_float4x4_t(.5f, 0.0f, 0.0f, 0.0f, 0.0f, .5f, 0.0f, 0.0f, 0.0f, 0.0f, .5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
        }
        else
        {
            auto view = skr::float4x4::view_at(
                mp_camera->position,
                mp_camera->position + mp_camera->front,
                mp_camera->up);

            auto proj = skr::float4x4::perspective_fov(
                skr::camera_fov_y_from_x(mp_camera->fov, mp_camera->aspect),
                mp_camera->aspect,
                mp_camera->near_plane,
                mp_camera->far_plane);
            auto _view_proj = skr::mul(view, proj);
            view_proj = skr::transpose(_view_proj);
        }

        skr::Vector<skr_primitive_draw_t> drawcalls;
        // drawcalls.reserve(render_mesh->primitive_commands.size());
        for (const auto& cmd : cmds)
        {
            skr_primitive_draw_t& drawcall = drawcalls.emplace().ref();
            drawcall.pipeline = pipeline;
            drawcall.push_const_name = push_constants_name;
            drawcall.push_const = (const uint8_t*)(&view_proj);
            drawcall.vertex_buffer_count = (uint32_t)cmd.vbvs.size();
            drawcall.vertex_buffers = cmd.vbvs.data();
            drawcall.index_buffer = *cmd.ibv;
        }
        // SKR_LOG_INFO(u8"Render Mesh has %d drawcalls", drawcalls.size());

        auto backbuffer = render_graph->get_texture(u8"backbuffer");
        const auto back_desc = render_graph->resolve_descriptor(backbuffer);
        render_graph->add_render_pass(
            [this, backbuffer](skr::render_graph::RenderGraph& g, skr::render_graph::RenderPassBuilder& builder) {
                builder.set_name(u8"scene_render_pass")
                    .set_pipeline(pipeline) // captured this->pipeline
                    .write(0, backbuffer, CGPU_LOAD_ACTION_CLEAR);
            },
            [back_desc, drawcalls](skr::render_graph::RenderGraph& g, skr::render_graph::RenderPassContext& context) {
                {
                    // do render pass
                    cgpu_render_encoder_set_viewport(context.encoder, 0.0, 0.0, (float)back_desc->width, (float)back_desc->height, 0.0f, 1.0f);
                    cgpu_render_encoder_set_scissor(context.encoder, 0, 0, back_desc->width, back_desc->height);

                    for (const auto& drawcall : drawcalls)
                    {
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
                }
            });
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