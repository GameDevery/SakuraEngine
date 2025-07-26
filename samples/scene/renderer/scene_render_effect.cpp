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

    void create_resources(skr::RendererDevice* render_device)
    {
        const skr_float3_t g_Positions[3] = {
            { -0.5f, -0.5f, 0.0f },
            { 0.5f, -0.5f, 0.0f },
            { 0.0f, 0.5f, 0.0f }
        };
        const skr_float3_t g_Colors[3] = {
            { 1.0f, 0.0f, 0.0f },
            { 0.0f, 1.0f, 0.0f },
            { 0.0f, 0.0f, 1.0f }
        };
        const uint32_t g_Indices[] = {
            0, 1, 2
        };

        auto cgpu_device = render_device->get_cgpu_device();
        CGPUBufferDescriptor upload_buffer_desc = {};
        upload_buffer_desc.name = u8"UploadBuffer";
        upload_buffer_desc.flags = CGPU_BCF_PERSISTENT_MAP_BIT;
        upload_buffer_desc.descriptors = CGPU_RESOURCE_TYPE_NONE;
        upload_buffer_desc.memory_usage = CGPU_MEM_USAGE_CPU_ONLY;
        upload_buffer_desc.size = sizeof(g_Positions) + sizeof(g_Colors) + sizeof(g_Indices) + sizeof(skr_float4x4_t) * 1; // one instance
        auto upload_buffer = cgpu_create_buffer(cgpu_device, &upload_buffer_desc);
        CGPUBufferDescriptor vertex_buffer_desc = {};
        vertex_buffer_desc.name = u8"VertexBuffer";
        vertex_buffer_desc.flags = CGPU_BCF_NONE;
        vertex_buffer_desc.descriptors = CGPU_RESOURCE_TYPE_VERTEX_BUFFER;
        vertex_buffer_desc.memory_usage = CGPU_MEM_USAGE_GPU_ONLY;
        vertex_buffer_desc.size = sizeof(g_Positions) + sizeof(g_Colors);
        vertex_buffer = cgpu_create_buffer(cgpu_device, &vertex_buffer_desc);
        CGPUBufferDescriptor index_buffer_desc = {};
        index_buffer_desc.name = u8"IndexBuffer";
        index_buffer_desc.flags = CGPU_BCF_NONE;
        index_buffer_desc.descriptors = CGPU_RESOURCE_TYPE_INDEX_BUFFER;
        index_buffer_desc.memory_usage = CGPU_MEM_USAGE_GPU_ONLY;
        index_buffer = cgpu_create_buffer(cgpu_device, &index_buffer_desc);
        CGPUBufferDescriptor instance_buffer_desc = {};
        instance_buffer_desc.name = u8"InstanceBuffer";
        instance_buffer_desc.flags = CGPU_BCF_NONE;
        instance_buffer_desc.descriptors = CGPU_RESOURCE_TYPE_VERTEX_BUFFER;
        instance_buffer_desc.memory_usage = CGPU_MEM_USAGE_GPU_ONLY;
        instance_buffer_desc.size = 1 * sizeof(skr_float4x4_t);
        instance_buffer = cgpu_create_buffer(cgpu_device, &instance_buffer_desc);

        // Create Copy and Upload Command
        auto gfx_queue = render_device->get_gfx_queue();
        auto pool_desc = CGPUCommandPoolDescriptor();
        auto cmd_pool = cgpu_create_command_pool(gfx_queue, &pool_desc);
        auto cmd_desc = CGPUCommandBufferDescriptor();
        auto cpy_cmd = cgpu_create_command_buffer(cmd_pool, &cmd_desc);
        {
            // copy vertex data to upload_buffer->info->cpu_mapped_address
        }
        cgpu_cmd_begin(cpy_cmd);

        // copy CPU upload_buffer to GPU vertex buffer
        CGPUBufferToBufferTransfer vb_cpy{};
        vb_cpy.dst = vertex_buffer;
        vb_cpy.dst_offset = 0;
        vb_cpy.src = upload_buffer;
        vb_cpy.src_offset = 0;
        vb_cpy.size = sizeof(g_Positions) + sizeof(g_Colors);
        cgpu_cmd_transfer_buffer_to_buffer(cpy_cmd, &vb_cpy);

        {
            // copy index data to upload_buffer->info->cpu_mapped_address + sizeof(g_Positions) + sizeof(g_Colors)
        }

        CGPUBufferToBufferTransfer ib_cpy{};
        ib_cpy.dst = index_buffer;
        ib_cpy.dst_offset = 0;
        ib_cpy.src = upload_buffer;
        ib_cpy.src_offset = sizeof(g_Positions) + sizeof(g_Colors);
        ib_cpy.size = sizeof(g_Indices);
        cgpu_cmd_transfer_buffer_to_buffer(cpy_cmd, &ib_cpy);

        {
            // copy instance data to upload_buffer->info->cpu_mapped_address + sizeof(g_Positions) + sizeof(g_Colors) + sizeof(g_Indices)
        }

        CGPUBufferToBufferTransfer istb_cpy{};
        istb_cpy.dst = instance_buffer;
        istb_cpy.dst_offset = 0;
        istb_cpy.src = upload_buffer;
        istb_cpy.src_offset = sizeof(g_Positions) + sizeof(g_Colors) + sizeof(g_Indices);
        istb_cpy.size = 1 * sizeof(skr_float4x4_t);
        cgpu_cmd_transfer_buffer_to_buffer(cpy_cmd, &istb_cpy);

        // Command Barriers
        CGPUBufferBarrier barriers[3] = {};

        CGPUBufferBarrier& vb_barrier = barriers[0];
        vb_barrier.buffer = vertex_buffer;
        vb_barrier.src_state = CGPU_RESOURCE_STATE_COPY_DEST;
        vb_barrier.dst_state = CGPU_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

        CGPUBufferBarrier& ib_barrier = barriers[1];
        ib_barrier.buffer = index_buffer;
        ib_barrier.src_state = CGPU_RESOURCE_STATE_COPY_DEST;
        ib_barrier.dst_state = CGPU_RESOURCE_STATE_INDEX_BUFFER;

        CGPUBufferBarrier& ist_barrier = barriers[2];
        ist_barrier.buffer = instance_buffer;
        ist_barrier.src_state = CGPU_RESOURCE_STATE_COPY_DEST;
        ist_barrier.dst_state = CGPU_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

        CGPUResourceBarrierDescriptor barrier_desc = {};
        barrier_desc.buffer_barriers = barriers;
        barrier_desc.buffer_barriers_count = 3;
        cgpu_cmd_resource_barrier(cpy_cmd, &barrier_desc);
        cgpu_cmd_end(cpy_cmd);

        // submit command to queue
        CGPUQueueSubmitDescriptor cpy_submit = {};
        cpy_submit.cmds = &cpy_cmd;
        cpy_submit.cmds_count = 1;
        cgpu_submit_queue(gfx_queue, &cpy_submit);
        cgpu_wait_queue_idle(gfx_queue);
        cgpu_free_buffer(upload_buffer);
        cgpu_free_command_buffer(cpy_cmd);
        cgpu_free_command_pool(cmd_pool);
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