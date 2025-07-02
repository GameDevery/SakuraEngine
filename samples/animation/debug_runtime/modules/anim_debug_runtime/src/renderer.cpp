#include "AnimDebugRuntime/renderer.h"
#include "common/utils.h"
#include "AnimDebugRuntime/cube_geometry.h"

namespace animd
{

LightingPushConstants   Renderer::lighting_data    = { 0, 0 };
LightingCSPushConstants Renderer::lighting_cs_data = { { 0, 0 }, { 0, 0 } };

CubeGeometry::InstanceData CubeGeometry::instance_data;

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
    // upload
    CGPUBufferDescriptor upload_buffer_desc = {};
    upload_buffer_desc.name                 = u8"UploadBuffer";
    upload_buffer_desc.flags                = CGPU_BCF_PERSISTENT_MAP_BIT;
    upload_buffer_desc.descriptors          = CGPU_RESOURCE_TYPE_NONE;
    upload_buffer_desc.memory_usage         = CGPU_MEM_USAGE_CPU_ONLY;
    upload_buffer_desc.size                 = sizeof(CubeGeometry) + sizeof(CubeGeometry::g_Indices) + sizeof(CubeGeometry::InstanceData);
    auto upload_buffer                      = cgpu_create_buffer(_device, &upload_buffer_desc);

    CGPUBufferDescriptor vb_desc = {};
    vb_desc.name                 = u8"VertexBuffer";
    vb_desc.flags                = CGPU_BCF_NONE;
    vb_desc.descriptors          = CGPU_RESOURCE_TYPE_VERTEX_BUFFER;
    vb_desc.memory_usage         = CGPU_MEM_USAGE_GPU_ONLY;
    vb_desc.size                 = sizeof(CubeGeometry);
    _vertex_buffer               = cgpu_create_buffer(_device, &vb_desc);

    CGPUBufferDescriptor ib_desc = {};
    ib_desc.name                 = u8"IndexBuffer";
    ib_desc.flags                = CGPU_BCF_NONE;
    ib_desc.descriptors          = CGPU_RESOURCE_TYPE_INDEX_BUFFER;
    ib_desc.memory_usage         = CGPU_MEM_USAGE_GPU_ONLY;
    ib_desc.size                 = sizeof(CubeGeometry::g_Indices);
    _index_buffer                = cgpu_create_buffer(_device, &ib_desc);

    CGPUBufferDescriptor inb_desc = {};
    inb_desc.name                 = u8"InstanceBuffer";
    inb_desc.flags                = CGPU_BCF_NONE;
    inb_desc.descriptors          = CGPU_RESOURCE_TYPE_VERTEX_BUFFER;
    inb_desc.memory_usage         = CGPU_MEM_USAGE_GPU_ONLY;
    inb_desc.size                 = sizeof(CubeGeometry::InstanceData);
    _instance_buffer              = cgpu_create_buffer(_device, &inb_desc);

    auto pool_desc = CGPUCommandPoolDescriptor();
    auto cmd_pool  = cgpu_create_command_pool(_gfx_queue, &pool_desc);
    auto cmd_desc  = CGPUCommandBufferDescriptor();
    auto cpy_cmd   = cgpu_create_command_buffer(cmd_pool, &cmd_desc);
    {
        auto geom = CubeGeometry();
        memcpy(upload_buffer->info->cpu_mapped_address, &geom, upload_buffer_desc.size);
    }
    cgpu_cmd_begin(cpy_cmd);
    CGPUBufferToBufferTransfer vb_cpy = {};
    vb_cpy.dst                        = _vertex_buffer;
    vb_cpy.dst_offset                 = 0;
    vb_cpy.src                        = upload_buffer;
    vb_cpy.src_offset                 = 0;
    vb_cpy.size                       = sizeof(CubeGeometry);
    cgpu_cmd_transfer_buffer_to_buffer(cpy_cmd, &vb_cpy);

    // copy geometry data to upload buffer
    {
        memcpy((char8_t*)upload_buffer->info->cpu_mapped_address + sizeof(CubeGeometry), CubeGeometry::g_Indices, sizeof(CubeGeometry::g_Indices));
    }

    // data[offset:offset+size] to _index_buffer
    CGPUBufferToBufferTransfer ib_cpy = {};
    ib_cpy.dst                        = _index_buffer;
    ib_cpy.dst_offset                 = 0;
    ib_cpy.src                        = upload_buffer;
    ib_cpy.src_offset                 = sizeof(CubeGeometry);
    ib_cpy.size                       = sizeof(CubeGeometry::g_Indices);
    cgpu_cmd_transfer_buffer_to_buffer(cpy_cmd, &ib_cpy);
    // wvp
    const auto quat = rtm::quat_from_euler(
        rtm::scalar_deg_to_rad(0.f),
        rtm::scalar_deg_to_rad(0.f),
        rtm::scalar_deg_to_rad(0.f)
    );
    const rtm::vector4f   translation = rtm::vector_set(0.f, 0.f, 0.f, 0.f);
    const rtm::vector4f   scale       = rtm::vector_set(2.f, 2.f, 2.f, 0.f);
    const rtm::qvvf       transform   = rtm::qvv_set(quat, translation, scale);
    const rtm::matrix4x4f matrix      = rtm::matrix_cast(rtm::matrix_from_qvv(transform));
    CubeGeometry::instance_data.world = *(skr_float4x4_t*)&matrix;
    {
        memcpy((char8_t*)upload_buffer->info->cpu_mapped_address + sizeof(CubeGeometry) + sizeof(CubeGeometry::g_Indices), &CubeGeometry::instance_data, sizeof(CubeGeometry::InstanceData));
    }

    CGPUBufferToBufferTransfer istb_cpy = {};
    istb_cpy.dst                        = _instance_buffer;
    istb_cpy.dst_offset                 = 0;
    istb_cpy.src                        = upload_buffer;
    istb_cpy.src_offset                 = sizeof(CubeGeometry) + sizeof(CubeGeometry::g_Indices);
    istb_cpy.size                       = sizeof(CubeGeometry::instance_data);
    cgpu_cmd_transfer_buffer_to_buffer(cpy_cmd, &istb_cpy);

    // barriers
    CGPUBufferBarrier  barriers[3]             = {};
    CGPUBufferBarrier& vb_barrier              = barriers[0];
    vb_barrier.buffer                          = _vertex_buffer;
    vb_barrier.src_state                       = CGPU_RESOURCE_STATE_COPY_DEST;
    vb_barrier.dst_state                       = CGPU_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    CGPUBufferBarrier& ib_barrier              = barriers[1];
    ib_barrier.buffer                          = _index_buffer;
    ib_barrier.src_state                       = CGPU_RESOURCE_STATE_COPY_DEST;
    ib_barrier.dst_state                       = CGPU_RESOURCE_STATE_INDEX_BUFFER;
    CGPUBufferBarrier& ist_barrier             = barriers[2];
    ist_barrier.buffer                         = _instance_buffer;
    ist_barrier.src_state                      = CGPU_RESOURCE_STATE_COPY_DEST;
    ist_barrier.dst_state                      = CGPU_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
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
    create_gbuffer_pipeline();
    create_lighting_pipeline();
    create_blit_pipeline();
}

void Renderer::create_lighting_pipeline()
{
    uint32_t *vs_bytes, vs_length;
    uint32_t *fs_bytes, fs_length;
    read_shader_bytes(SKR_UTF8("AnimDebug/screen_vs"), &vs_bytes, &vs_length, _device->adapter->instance->backend);
    read_shader_bytes(SKR_UTF8("AnimDebug/lighting_fs"), &fs_bytes, &fs_length, _device->adapter->instance->backend);
    CGPUShaderLibraryDescriptor vs_desc = {};
    vs_desc.stage                       = CGPU_SHADER_STAGE_VERT;
    vs_desc.name                        = SKR_UTF8("ScreenVertexShader");
    vs_desc.code                        = vs_bytes;
    vs_desc.code_size                   = vs_length;
    CGPUShaderLibraryDescriptor ps_desc = {};
    ps_desc.name                        = SKR_UTF8("LightingFragmentShader");
    ps_desc.stage                       = CGPU_SHADER_STAGE_FRAG;
    ps_desc.code                        = fs_bytes;
    ps_desc.code_size                   = fs_length;
    auto screen_vs                      = cgpu_create_shader_library(_device, &vs_desc);
    auto lighting_fs                    = cgpu_create_shader_library(_device, &ps_desc);
    free(vs_bytes);
    free(fs_bytes);
    CGPUShaderEntryDescriptor ppl_shaders[2];
    ppl_shaders[0].stage                            = CGPU_SHADER_STAGE_VERT;
    ppl_shaders[0].entry                            = SKR_UTF8("main");
    ppl_shaders[0].library                          = screen_vs;
    ppl_shaders[1].stage                            = CGPU_SHADER_STAGE_FRAG;
    ppl_shaders[1].entry                            = SKR_UTF8("main");
    ppl_shaders[1].library                          = lighting_fs;
    const char8_t*              push_constant_name  = SKR_UTF8("push_constants");
    const char8_t*              static_sampler_name = SKR_UTF8("texture_sampler");
    CGPURootSignatureDescriptor rs_desc             = {};
    rs_desc.shaders                                 = ppl_shaders;
    rs_desc.shader_count                            = 2;
    rs_desc.push_constant_count                     = 1;
    rs_desc.push_constant_names                     = &push_constant_name;
    rs_desc.static_sampler_count                    = 1;
    rs_desc.static_sampler_names                    = &static_sampler_name;
    rs_desc.static_samplers                         = &_static_sampler;
    auto             lighting_root_sig              = cgpu_create_root_signature(_device, &rs_desc);
    CGPUVertexLayout vertex_layout                  = {};
    vertex_layout.attribute_count                   = 0;
    CGPURenderPipelineDescriptor rp_desc            = {};
    rp_desc.root_signature                          = lighting_root_sig;
    rp_desc.prim_topology                           = CGPU_PRIM_TOPO_TRI_LIST;
    rp_desc.vertex_layout                           = &vertex_layout;
    rp_desc.vertex_shader                           = &ppl_shaders[0];
    rp_desc.fragment_shader                         = &ppl_shaders[1];
    rp_desc.render_target_count                     = 1;
    auto backbuffer_format                          = CGPU_FORMAT_R8G8B8A8_UNORM;
    rp_desc.color_formats                           = &backbuffer_format;
    _lighting_pipeline                              = cgpu_create_render_pipeline(_device, &rp_desc);
    cgpu_free_shader_library(screen_vs);
    cgpu_free_shader_library(lighting_fs);
}
void Renderer::create_blit_pipeline()
{
    ECGPUFormat output_format = CGPU_FORMAT_R8G8B8A8_UNORM;
    uint32_t *  vs_bytes, vs_length;
    uint32_t *  fs_bytes, fs_length;
    read_shader_bytes(SKR_UTF8("AnimDebug/screen_vs"), &vs_bytes, &vs_length, _device->adapter->instance->backend);
    read_shader_bytes(SKR_UTF8("AnimDebug/blit_fs"), &fs_bytes, &fs_length, _device->adapter->instance->backend);
    CGPUShaderLibraryDescriptor vs_desc = {};
    vs_desc.stage                       = CGPU_SHADER_STAGE_VERT;
    vs_desc.name                        = SKR_UTF8("ScreenVertexShader");
    vs_desc.code                        = vs_bytes;
    vs_desc.code_size                   = vs_length;
    CGPUShaderLibraryDescriptor ps_desc = {};
    ps_desc.name                        = SKR_UTF8("BlitFragmentShader");
    ps_desc.stage                       = CGPU_SHADER_STAGE_FRAG;
    ps_desc.code                        = fs_bytes;
    ps_desc.code_size                   = fs_length;
    auto screen_vs                      = cgpu_create_shader_library(_device, &vs_desc);
    auto blit_fs                        = cgpu_create_shader_library(_device, &ps_desc);
    free(vs_bytes);
    free(fs_bytes);
    CGPUShaderEntryDescriptor ppl_shaders[2];
    ppl_shaders[0].stage                            = CGPU_SHADER_STAGE_VERT;
    ppl_shaders[0].entry                            = SKR_UTF8("main");
    ppl_shaders[0].library                          = screen_vs;
    ppl_shaders[1].stage                            = CGPU_SHADER_STAGE_FRAG;
    ppl_shaders[1].entry                            = SKR_UTF8("main");
    ppl_shaders[1].library                          = blit_fs;
    const char8_t*              static_sampler_name = SKR_UTF8("texture_sampler");
    CGPURootSignatureDescriptor rs_desc             = {};
    rs_desc.shaders                                 = ppl_shaders;
    rs_desc.shader_count                            = 2;
    rs_desc.static_sampler_count                    = 1;
    rs_desc.static_sampler_names                    = &static_sampler_name;
    rs_desc.static_samplers                         = &_static_sampler;
    auto             lighting_root_sig              = cgpu_create_root_signature(_device, &rs_desc);
    CGPUVertexLayout vertex_layout                  = {};
    vertex_layout.attribute_count                   = 0;
    CGPURenderPipelineDescriptor rp_desc            = {};
    rp_desc.root_signature                          = lighting_root_sig;
    rp_desc.prim_topology                           = CGPU_PRIM_TOPO_TRI_LIST;
    rp_desc.vertex_layout                           = &vertex_layout;
    rp_desc.vertex_shader                           = &ppl_shaders[0];
    rp_desc.fragment_shader                         = &ppl_shaders[1];
    rp_desc.render_target_count                     = 1;
    rp_desc.color_formats                           = &output_format;
    _blit_pipeline                                  = cgpu_create_render_pipeline(_device, &rp_desc);
    cgpu_free_shader_library(screen_vs);
    cgpu_free_shader_library(blit_fs);
}

void Renderer::create_gbuffer_pipeline()
{
    uint32_t *vs_bytes, vs_length;
    uint32_t *fs_bytes, fs_length;
    read_shader_bytes(SKR_UTF8("AnimDebug/gbuffer_vs"), &vs_bytes, &vs_length, _device->adapter->instance->backend);
    read_shader_bytes(SKR_UTF8("AnimDebug/gbuffer_fs"), &fs_bytes, &fs_length, _device->adapter->instance->backend);
    CGPUShaderLibraryDescriptor vs_desc = {};
    vs_desc.stage                       = CGPU_SHADER_STAGE_VERT;
    vs_desc.name                        = SKR_UTF8("GBufferVertexShader");
    vs_desc.code                        = vs_bytes;
    vs_desc.code_size                   = vs_length;
    CGPUShaderLibraryDescriptor ps_desc = {};
    ps_desc.name                        = SKR_UTF8("GBufferFragmentShader");
    ps_desc.stage                       = CGPU_SHADER_STAGE_FRAG;
    ps_desc.code                        = fs_bytes;
    ps_desc.code_size                   = fs_length;
    CGPUShaderLibraryId gbuffer_vs      = cgpu_create_shader_library(_device, &vs_desc);
    CGPUShaderLibraryId gbuffer_fs      = cgpu_create_shader_library(_device, &ps_desc);
    free(vs_bytes);
    free(fs_bytes);
    CGPUShaderEntryDescriptor ppl_shaders[2];
    ppl_shaders[0].stage                = CGPU_SHADER_STAGE_VERT;
    ppl_shaders[0].entry                = SKR_UTF8("main");
    ppl_shaders[0].library              = gbuffer_vs;
    ppl_shaders[1].stage                = CGPU_SHADER_STAGE_FRAG;
    ppl_shaders[1].entry                = SKR_UTF8("main");
    ppl_shaders[1].library              = gbuffer_fs;
    CGPURootSignatureDescriptor rs_desc = {};
    rs_desc.shaders                     = ppl_shaders;
    rs_desc.shader_count                = 2;
    rs_desc.push_constant_count         = 1;
    const char8_t* root_const_name      = SKR_UTF8("push_constants");
    rs_desc.push_constant_names         = &root_const_name;
    auto gbuffer_root_sig               = cgpu_create_root_signature(_device, &rs_desc);

    CGPUVertexLayout vertex_layout = {};
    vertex_layout.attributes[0]    = { SKR_UTF8("POSITION"), 1, CGPU_FORMAT_R32G32B32_SFLOAT, 0, 0, sizeof(skr_float3_t), CGPU_INPUT_RATE_VERTEX };
    vertex_layout.attributes[1]    = { SKR_UTF8("TEXCOORD"), 1, CGPU_FORMAT_R32G32_SFLOAT, 1, 0, sizeof(skr_float2_t), CGPU_INPUT_RATE_VERTEX };
    vertex_layout.attributes[2]    = { SKR_UTF8("NORMAL"), 1, CGPU_FORMAT_R8G8B8A8_SNORM, 2, 0, sizeof(uint32_t), CGPU_INPUT_RATE_VERTEX };
    vertex_layout.attributes[3]    = { SKR_UTF8("TANGENT"), 1, CGPU_FORMAT_R8G8B8A8_SNORM, 3, 0, sizeof(uint32_t), CGPU_INPUT_RATE_VERTEX };
    vertex_layout.attributes[4]    = { SKR_UTF8("MODEL"), 4, CGPU_FORMAT_R32G32B32A32_SFLOAT, 4, 0, sizeof(skr_float4x4_t), CGPU_INPUT_RATE_INSTANCE };
    vertex_layout.attribute_count  = 5;

    CGPURenderPipelineDescriptor rp_desc      = {};
    rp_desc.root_signature                    = gbuffer_root_sig;
    rp_desc.prim_topology                     = CGPU_PRIM_TOPO_TRI_LIST;
    rp_desc.vertex_layout                     = &vertex_layout;
    rp_desc.vertex_shader                     = &ppl_shaders[0];
    rp_desc.fragment_shader                   = &ppl_shaders[1];
    rp_desc.render_target_count               = sizeof(gbuffer_formats) / sizeof(ECGPUFormat);
    rp_desc.color_formats                     = gbuffer_formats;
    rp_desc.depth_stencil_format              = gbuffer_depth_format;
    CGPURasterizerStateDescriptor raster_desc = {};
    raster_desc.cull_mode                     = ECGPUCullMode::CGPU_CULL_MODE_BACK;
    raster_desc.depth_bias                    = 0;
    raster_desc.fill_mode                     = CGPU_FILL_MODE_SOLID;
    raster_desc.front_face                    = CGPU_FRONT_FACE_CCW;
    rp_desc.rasterizer_state                  = &raster_desc;
    CGPUDepthStateDescriptor ds_desc          = {};
    ds_desc.depth_func                        = CGPU_CMP_LEQUAL;
    ds_desc.depth_write                       = true;
    ds_desc.depth_test                        = true;
    rp_desc.depth_state                       = &ds_desc;
    _gbuffer_pipeline                         = cgpu_create_render_pipeline(_device, &rp_desc);
    cgpu_free_shader_library(gbuffer_vs);
    cgpu_free_shader_library(gbuffer_fs);
}

void Renderer::build_render_graph(skr::render_graph::RenderGraph* graph, skr::render_graph::TextureHandle back_buffer)
{
    namespace rg = skr::render_graph;

    rg::TextureHandle composite_buffer = graph->create_texture(
        [this](rg::RenderGraph& g, rg::TextureBuilder& builder) {
            builder.set_name(u8"composite_buffer")
                .extent(_width, _height)
                .format(CGPU_FORMAT_R8G8B8A8_UNORM)
                .allocate_dedicated()
                .allow_render_target();
        }
    );
    auto gbuffer_color = graph->create_texture(
        [this](rg::RenderGraph& g, rg::TextureBuilder& builder) {
            builder.set_name(u8"gbuffer_color")
                .extent(_width, _height)
                .format(CGPU_FORMAT_R8G8B8A8_UNORM)
                .allocate_dedicated()
                .allow_render_target();
        }
    );
    auto gbuffer_depth = graph->create_texture(
        [this](rg::RenderGraph& g, rg::TextureBuilder& builder) {
            builder.set_name(u8"gbuffer_depth")
                .extent(_width, _height)
                .format(CGPU_FORMAT_D32_SFLOAT)
                .allocate_dedicated()
                .allow_depth_stencil();
        }
    );
    auto gbuffer_normal = graph->create_texture(
        [this](rg::RenderGraph& g, rg::TextureBuilder& builder) {
            builder.set_name(u8"gbuffer_normal")
                .extent(_width, _height)
                .format(CGPU_FORMAT_R16G16B16A16_SNORM)
                .allocate_dedicated()
                .allow_render_target();
        }
    );
    // camera
    auto view = rtm::view_look_at(
        rtm::vector_set(0.f, 2.5f, 2.5f, 0.0f) /*eye*/,
        rtm::vector_set(0.f, 0.f, 0.f, 0.f) /*at*/,
        rtm::vector_set(0.f, 1.f, 0.f, 0.f) /*up*/
    );
    auto proj = rtm::proj_perspective_fov(
        3.1415926f / 2.f,
        (float)BACK_BUFFER_WIDTH / (float)BACK_BUFFER_HEIGHT,
        1.f, 1000.f
    );
    auto view_proj = rtm::matrix_mul(rtm::matrix_cast(view), proj);

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
            cgpu_render_encoder_set_viewport(stack.encoder, 0.0f, 0.0f, (float)_width, (float)_height, 0.f, 1.f);
            cgpu_render_encoder_set_scissor(stack.encoder, 0, 0, _width, _height);

            CGPUBufferId vertex_buffers[5] = {
                _vertex_buffer, _vertex_buffer, _vertex_buffer,
                _vertex_buffer, _instance_buffer
            };
            const uint32_t strides[5] = {
                sizeof(skr_float3_t), sizeof(skr_float2_t),
                sizeof(uint32_t), sizeof(uint32_t),
                sizeof(animd::CubeGeometry::InstanceData::world)
            };
            const uint32_t offsets[5] = {
                offsetof(CubeGeometry, g_Positions), offsetof(CubeGeometry, g_TexCoords),
                offsetof(CubeGeometry, g_Normals), offsetof(CubeGeometry, g_Tangents),
                offsetof(CubeGeometry::InstanceData, world)
            };
            cgpu_render_encoder_bind_index_buffer(stack.encoder, _index_buffer, sizeof(uint32_t), 0);
            cgpu_render_encoder_bind_vertex_buffers(stack.encoder, 5, vertex_buffers, strides, offsets);
            cgpu_render_encoder_push_constants(stack.encoder, _gbuffer_pipeline->root_signature, u8"push_constants", &view_proj);
            cgpu_render_encoder_draw_indexed_instanced(stack.encoder, 36, 0, 1, 0, 0);
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
            cgpu_render_encoder_set_viewport(stack.encoder, 0.0f, 0.0f, (float)_width, (float)_height, 0.f, 1.f);
            cgpu_render_encoder_set_scissor(stack.encoder, 0, 0, _width, _height);
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
            cgpu_render_encoder_set_viewport(stack.encoder, 0.0f, 0.0f, (float)_width, (float)_height, 0.f, 1.f);
            cgpu_render_encoder_set_scissor(stack.encoder, 0, 0, _width, _height);
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