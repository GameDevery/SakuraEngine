#include "AnimDebugRuntime/renderer.h"
#include "common/utils.h"
// #include "AnimDebugRuntime/cube_geometry.h"
#include "AnimDebugRuntime/bone_geometry.h"
#include "SkrCore/memory/memory.h"
#include <SkrAnim/ozz/skeleton.h>
#include <SkrAnim/ozz/base/io/archive.h>
#include <SkrAnim/ozz/skeleton_utils.h>
#include <SkrAnim/ozz/local_to_model_job.h>
#include <AnimDebugRuntime/util.h>
#include <SkrAnim/ozz/base/containers/vector.h>

namespace animd
{

LightingPushConstants   Renderer::lighting_data    = { 0, 0 };
LightingCSPushConstants Renderer::lighting_cs_data = { { 0, 0 }, { 0, 0 } };

void Renderer::read_anim()
{

    // ozz::animation::Skeleton skeleton;

    // auto          filename = "D:/ws/data/assets/media/bin/pab_skeleton.ozz";
    // ozz::io::File file(filename, "rb");
    // if (!file.opened())
    // {
    //     SKR_LOG_ERROR(u8"Cannot open file %s.", filename);
    //     return;
    // }

    // // deserialize
    // ozz::io::IArchive archive(&file);
    // // test archive
    // if (!archive.TestTag<ozz::animation::Skeleton>())
    // {
    //     SKR_LOG_ERROR(u8"Archive doesn't contain the expected object type.");
    //     return;
    // }
    // // Create Runtime Skeleton
    // archive >> skeleton;

    // SKR_LOG_INFO(u8"Skeleton loaded with %d joints.", skeleton.num_joints());

    // ozz::vector<ozz::math::Float4x4> prealloc_models_;
    // prealloc_models_.resize(skeleton.num_joints());
    // ozz::animation::LocalToModelJob job;
    // job.input    = skeleton.joint_rest_poses();
    // job.output   = ozz::make_span(prealloc_models_);
    // job.skeleton = &skeleton;
    // if (!job.Run())
    // {
    //     SKR_LOG_ERROR(u8"Failed to run LocalToModelJob.");
    // }
    // _instance_count = skeleton.num_joints();
    // // _instance_count = 2;
    // _instance_data.resize(_instance_count, skr::float4x4::identity());

    // const auto resize_transform = skr::TransformF(skr::QuatF(skr::RotatorF()), skr::float3(1), skr::float3(0.1f));
    // const auto resize_matrix    = skr::transpose(resize_transform.to_matrix());
    // // _instance_data[1]    = *(skr_float4x4_t*)&matrix;

    // for (int i = 0; i < _instance_count; ++i)
    // {
    //     ozz::math::Float4x4 model_matrix = prealloc_models_[i];
    //     auto                mat          = animd::ozz2skr_float4x4(model_matrix);
    //     // auto                mat          = skr::float4x4::identity();
    //     auto parent_id = skeleton.joint_parents()[i];
    //     if (parent_id < 0)
    //     {
    //         continue; // skip root joint
    //     }
    //     _instance_data[i] = *(skr_float4x4_t*)&mat;
    // }

    _instance_count = 2;
    _instance_data.resize(_instance_count, skr::float4x4::identity());
    const auto t0     = skr::TransformF(skr::QuatF(skr::RotatorF()), skr::float3(0.5), skr::float3(0.8f));
    const auto m0     = skr::transpose(t0.to_matrix());
    _instance_data[0] = *(skr_float4x4_t*)&m0;
    const auto t1     = skr::TransformF(skr::QuatF(skr::RotatorF()), skr::float3(1, 0, 0), skr::float3(0.8f));
    const auto m1     = skr::transpose(t1.to_matrix());
    _instance_data[1] = *(skr_float4x4_t*)&m1;
}

void Renderer::create_api_objects()
{
    {
        // Create instance
        CGPUInstanceDescriptor instance_desc{};
        instance_desc.backend                     = _backend;
        instance_desc.enable_debug_layer          = true;
        instance_desc.enable_gpu_based_validation = false;
        instance_desc.enable_set_name             = true;

        _instance = cgpu_create_instance(&instance_desc);
    }
    {
        // Select Adapter
        uint32_t adapters_count = 0;
        cgpu_enum_adapters(_instance, CGPU_NULLPTR, &adapters_count);
        CGPUAdapterId adapters[64];
        cgpu_enum_adapters(_instance, adapters, &adapters_count);
        _adapter = adapters[0];
    }

    {
        // create device and queue
        CGPUQueueGroupDescriptor queue_group_desc = {};
        queue_group_desc.queue_type               = CGPU_QUEUE_TYPE_GRAPHICS;
        queue_group_desc.queue_count              = 1;
        CGPUDeviceDescriptor device_desc          = {};
        device_desc.queue_groups                  = &queue_group_desc;
        device_desc.queue_group_count             = 1;
        _device                                   = cgpu_create_device(_adapter, &device_desc);
        _gfx_queue                                = cgpu_get_queue(_device, CGPU_QUEUE_TYPE_GRAPHICS, 0);
    }

    {
        CGPUSamplerDescriptor sampler_desc = {};
        sampler_desc.address_u             = CGPU_ADDRESS_MODE_REPEAT;
        sampler_desc.address_v             = CGPU_ADDRESS_MODE_REPEAT;
        sampler_desc.address_w             = CGPU_ADDRESS_MODE_REPEAT;
        sampler_desc.mipmap_mode           = CGPU_MIPMAP_MODE_LINEAR;
        sampler_desc.min_filter            = CGPU_FILTER_TYPE_LINEAR;
        sampler_desc.mag_filter            = CGPU_FILTER_TYPE_LINEAR;
        sampler_desc.compare_func          = CGPU_CMP_NEVER;
        _static_sampler                    = cgpu_create_sampler(_device, &sampler_desc);
    }
}
void Renderer::create_resources()
{
    CGPUBufferDescriptor upload_buffer_desc = {};
    upload_buffer_desc.name                 = u8"UploadBuffer";
    upload_buffer_desc.flags                = CGPU_BCF_PERSISTENT_MAP_BIT;
    upload_buffer_desc.descriptors          = CGPU_RESOURCE_TYPE_NONE;
    upload_buffer_desc.memory_usage         = CGPU_MEM_USAGE_CPU_ONLY;

    upload_buffer_desc.size = sizeof(BoneGeometry) + sizeof(BoneGeometry::g_Indices) + _instance_count * sizeof(skr_float4x4_t);
    auto upload_buffer      = cgpu_create_buffer(_device, &upload_buffer_desc);

    CGPUBufferDescriptor vb_desc = {};
    vb_desc.name                 = u8"VertexBuffer";
    vb_desc.flags                = CGPU_BCF_NONE;
    vb_desc.descriptors          = CGPU_RESOURCE_TYPE_VERTEX_BUFFER;
    vb_desc.memory_usage         = CGPU_MEM_USAGE_GPU_ONLY;
    vb_desc.size                 = sizeof(BoneGeometry);

    _vertex_buffer = cgpu_create_buffer(_device, &vb_desc);

    CGPUBufferDescriptor ib_desc = {};
    ib_desc.name                 = u8"IndexBuffer";
    ib_desc.flags                = CGPU_BCF_NONE;
    ib_desc.descriptors          = CGPU_RESOURCE_TYPE_INDEX_BUFFER;
    ib_desc.memory_usage         = CGPU_MEM_USAGE_GPU_ONLY;
    ib_desc.size                 = sizeof(BoneGeometry::g_Indices);

    _index_buffer = cgpu_create_buffer(_device, &ib_desc);

    CGPUBufferDescriptor inb_desc = {};
    inb_desc.name                 = u8"InstanceBuffer";
    inb_desc.flags                = CGPU_BCF_NONE;
    inb_desc.descriptors          = CGPU_RESOURCE_TYPE_VERTEX_BUFFER;
    inb_desc.memory_usage         = CGPU_MEM_USAGE_GPU_ONLY;
    inb_desc.size                 = _instance_count * sizeof(skr_float4x4_t);

    _instance_buffer = cgpu_create_buffer(_device, &inb_desc);

    auto pool_desc = CGPUCommandPoolDescriptor();
    auto cmd_pool  = cgpu_create_command_pool(_gfx_queue, &pool_desc);
    auto cmd_desc  = CGPUCommandBufferDescriptor();
    auto cpy_cmd   = cgpu_create_command_buffer(cmd_pool, &cmd_desc);
    {
        auto geom = BoneGeometry();
        memcpy(upload_buffer->info->cpu_mapped_address, &geom, sizeof(BoneGeometry));
    }
    cgpu_cmd_begin(cpy_cmd);
    CGPUBufferToBufferTransfer vb_cpy = {};
    vb_cpy.dst                        = _vertex_buffer;
    vb_cpy.dst_offset                 = 0;
    vb_cpy.src                        = upload_buffer;
    vb_cpy.src_offset                 = 0;
    // vb_cpy.size                       = sizeof(CubeGeometry);
    vb_cpy.size = sizeof(BoneGeometry);
    cgpu_cmd_transfer_buffer_to_buffer(cpy_cmd, &vb_cpy);

    // copy geometry data to upload buffer
    {
        memcpy((char8_t*)upload_buffer->info->cpu_mapped_address + sizeof(BoneGeometry), BoneGeometry::g_Indices, sizeof(BoneGeometry::g_Indices));
    }

    // data[offset:offset+size] to _index_buffer
    CGPUBufferToBufferTransfer ib_cpy = {};
    ib_cpy.dst                        = _index_buffer;
    ib_cpy.dst_offset                 = 0;
    ib_cpy.src                        = upload_buffer;
    // ib_cpy.src_offset                 = sizeof(CubeGeometry);
    // ib_cpy.size                       = sizeof(CubeGeometry::g_Indices);
    ib_cpy.src_offset = sizeof(BoneGeometry);
    ib_cpy.size       = sizeof(BoneGeometry::g_Indices);
    cgpu_cmd_transfer_buffer_to_buffer(cpy_cmd, &ib_cpy);
    // wvp
    {
        memcpy((char8_t*)upload_buffer->info->cpu_mapped_address + sizeof(BoneGeometry) + sizeof(BoneGeometry::g_Indices), _instance_data.data(), sizeof(skr_float4x4_t) * _instance_count);
    }

    CGPUBufferToBufferTransfer istb_cpy = {};
    istb_cpy.dst                        = _instance_buffer;
    istb_cpy.dst_offset                 = 0;
    istb_cpy.src                        = upload_buffer;
    istb_cpy.src_offset                 = sizeof(BoneGeometry) + sizeof(BoneGeometry::g_Indices);
    istb_cpy.size                       = _instance_count * sizeof(skr_float4x4_t);
    cgpu_cmd_transfer_buffer_to_buffer(cpy_cmd, &istb_cpy);

    // barriers
    CGPUBufferBarrier barriers[3] = {};

    CGPUBufferBarrier& vb_barrier = barriers[0];
    vb_barrier.buffer             = _vertex_buffer;
    vb_barrier.src_state          = CGPU_RESOURCE_STATE_COPY_DEST;
    vb_barrier.dst_state          = CGPU_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

    CGPUBufferBarrier& ib_barrier = barriers[1];
    ib_barrier.buffer             = _index_buffer;
    ib_barrier.src_state          = CGPU_RESOURCE_STATE_COPY_DEST;
    ib_barrier.dst_state          = CGPU_RESOURCE_STATE_INDEX_BUFFER;

    CGPUBufferBarrier& ist_barrier = barriers[2];
    ist_barrier.buffer             = _instance_buffer;
    ist_barrier.src_state          = CGPU_RESOURCE_STATE_COPY_DEST;
    ist_barrier.dst_state          = CGPU_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

    CGPUResourceBarrierDescriptor barrier_desc = {};
    barrier_desc.buffer_barriers               = barriers;
    barrier_desc.buffer_barriers_count         = 3;
    cgpu_cmd_resource_barrier(cpy_cmd, &barrier_desc);
    cgpu_cmd_end(cpy_cmd);

    // submit command to queue
    CGPUQueueSubmitDescriptor cpy_submit = {};
    cpy_submit.cmds                      = &cpy_cmd;
    cpy_submit.cmds_count                = 1;
    cgpu_submit_queue(_gfx_queue, &cpy_submit);
    cgpu_wait_queue_idle(_gfx_queue);
    cgpu_free_buffer(upload_buffer);
    cgpu_free_command_buffer(cpy_cmd);
    cgpu_free_command_pool(cmd_pool);
}

void Renderer::create_render_pipeline()
{
    create_debug_pipeline();
    create_gbuffer_pipeline();
    create_lighting_pipeline();
    create_blit_pipeline();
}

void Renderer::build_render_graph(skr::render_graph::RenderGraph* graph, skr::render_graph::TextureHandle back_buffer)
{
    namespace rg = skr::render_graph;

    rg::TextureHandle composite_buffer = graph->create_texture(
        [this](rg::RenderGraph& g, rg::TextureBuilder& builder) {
            builder.set_name(u8"composite_buffer")
                .extent(this->_width, this->_height)
                .format(CGPU_FORMAT_R8G8B8A8_UNORM)
                .allocate_dedicated()
                .allow_render_target();
        }
    );
    auto gbuffer_color = graph->create_texture(
        [this](rg::RenderGraph& g, rg::TextureBuilder& builder) {
            builder.set_name(u8"gbuffer_color")
                .extent(this->_width, this->_height)
                .format(CGPU_FORMAT_R8G8B8A8_UNORM)
                .allocate_dedicated()
                .allow_render_target();
        }
    );
    auto gbuffer_depth = graph->create_texture(
        [this](rg::RenderGraph& g, rg::TextureBuilder& builder) {
            builder.set_name(u8"gbuffer_depth")
                .extent(this->_width, this->_height)
                .format(CGPU_FORMAT_D32_SFLOAT)
                .allocate_dedicated()
                .allow_depth_stencil();
        }
    );
    auto gbuffer_normal = graph->create_texture(
        [this](rg::RenderGraph& g, rg::TextureBuilder& builder) {
            builder.set_name(u8"gbuffer_normal")
                .extent(this->_width, this->_height)
                .format(CGPU_FORMAT_R16G16B16A16_SNORM)
                .allocate_dedicated()
                .allow_render_target();
        }
    );

    // update camera view and projection matrices
    if (!mp_camera)
    {
        SKR_LOG_ERROR(u8"Camera is not set.");
        return;
    }
    auto view = skr::float4x4::view_at(skr::float4(mp_camera->position, 0.0f), skr::float4(mp_camera->position + mp_camera->front, 0.0f), skr::float4(mp_camera->up, 0.f));

    auto proj = skr::float4x4::perspective_fov(
        skr::camera_fov_y_from_x(mp_camera->fov, mp_camera->aspect),
        mp_camera->aspect,
        mp_camera->near_plane, mp_camera->far_plane
    );
    auto _view_proj = skr::mul(view, proj);
    auto view_proj  = skr::transpose(_view_proj);
    // render to g-buffer
    graph->add_render_pass(
        [this, gbuffer_color, gbuffer_normal, gbuffer_depth](rg::RenderGraph& g, rg::RenderPassBuilder& builder) {
            builder.set_name(u8"gbuffer_pass")
                .set_pipeline(_gbuffer_pipeline)
                .write(0, gbuffer_color, CGPU_LOAD_ACTION_CLEAR)
                .write(1, gbuffer_normal, CGPU_LOAD_ACTION_CLEAR)
                .set_depth_stencil(gbuffer_depth.clear_depth(1.f));
        },
        [this, view_proj](rg::RenderGraph& g, rg::RenderPassContext& stack) {
            cgpu_render_encoder_set_viewport(stack.encoder, 0.0f, 0.0f, (float)this->_width, (float)this->_height, 0.f, 1.f);
            cgpu_render_encoder_set_scissor(stack.encoder, 0, 0, this->_width, this->_height);

            CGPUBufferId vertex_buffers[5] = {
                _vertex_buffer, _vertex_buffer, _vertex_buffer,
                _vertex_buffer, _instance_buffer
            };

            const uint32_t strides[5] = {
                sizeof(skr_float3_t), sizeof(skr_float2_t),
                sizeof(uint32_t), sizeof(uint32_t),
                sizeof(skr_float4x4_t)
            };
            const uint32_t offsets[5] = {
                offsetof(BoneGeometry, g_Positions), offsetof(BoneGeometry, g_TexCoords),
                offsetof(BoneGeometry, g_Normals), offsetof(BoneGeometry, g_Tangents),
                0
            };
            cgpu_render_encoder_bind_index_buffer(stack.encoder, _index_buffer, sizeof(uint32_t), 0);
            cgpu_render_encoder_bind_vertex_buffers(stack.encoder, 5, vertex_buffers, strides, offsets);
            cgpu_render_encoder_push_constants(stack.encoder, _gbuffer_pipeline->root_signature, u8"push_constants", &view_proj);

            cgpu_render_encoder_draw_indexed_instanced(stack.encoder, 24, 0, _instance_count, 0, 0);
        }

    );
    // screen space lighting with fragment shader

    // pass by value, so that we can use when graph executing
    graph->add_render_pass(
        [this, gbuffer_color, gbuffer_normal, gbuffer_depth, composite_buffer](rg::RenderGraph& g, rg::RenderPassBuilder& builder) {
            builder.set_name(u8"light_pass_fs")
                .set_pipeline(_lighting_pipeline)
                .read(u8"gbuffer_color", gbuffer_color.read_mip(0, 1))
                .read(u8"gbuffer_normal", gbuffer_normal)
                .read(u8"gbuffer_depth", gbuffer_depth)
                .write(0, composite_buffer, CGPU_LOAD_ACTION_CLEAR);
        },
        [this](rg::RenderGraph& g, rg::RenderPassContext& stack) {
            cgpu_render_encoder_set_viewport(stack.encoder, 0.0f, 0.0f, (float)this->_width, (float)this->_height, 0.f, 1.f);
            cgpu_render_encoder_set_scissor(stack.encoder, 0, 0, this->_width, this->_height);
            cgpu_render_encoder_push_constants(stack.encoder, _lighting_pipeline->root_signature, u8"push_constants", &lighting_data);
            cgpu_render_encoder_draw(stack.encoder, 3, 0);
        }
    );

    graph->add_render_pass(
        [back_buffer, composite_buffer, this](rg::RenderGraph& g, rg::RenderPassBuilder& builder) {
            builder.set_name(u8"final_blit")
                .set_pipeline(this->_blit_pipeline)
                .read(u8"input_color", composite_buffer)
                .write(0, back_buffer, CGPU_LOAD_ACTION_CLEAR);
        },
        [this](rg::RenderGraph& g, rg::RenderPassContext& stack) {
            cgpu_render_encoder_set_viewport(stack.encoder, 0.0f, 0.0f, (float)this->_width, (float)this->_height, 0.f, 1.f);
            cgpu_render_encoder_set_scissor(stack.encoder, 0, 0, this->_width, this->_height);
            cgpu_render_encoder_draw(stack.encoder, 3, 0);
        }
    );

} // namespace skr::render_graph

void Renderer::finalize()
{
    // Free cgpu objects
    cgpu_free_buffer(_index_buffer);
    cgpu_free_buffer(_vertex_buffer);
    cgpu_free_buffer(_instance_buffer);
    free_pipeline_and_signature(_gbuffer_pipeline);
    free_pipeline_and_signature(_lighting_pipeline);
    free_pipeline_and_signature(_blit_pipeline);
    cgpu_free_sampler(_static_sampler);
    cgpu_free_queue(_gfx_queue);
    cgpu_free_device(_device);
    cgpu_free_instance(_instance);
}

} // namespace animd