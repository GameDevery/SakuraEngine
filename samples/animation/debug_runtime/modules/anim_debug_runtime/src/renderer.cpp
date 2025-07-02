#include "AnimDebugRuntime/renderer.h"
#include "common/utils.h"

namespace animd
{

LightingPushConstants   Renderer::lighting_data    = { 0, 0 };
LightingCSPushConstants Renderer::lighting_cs_data = { { 0, 0 }, { 0, 0 } };

void Renderer::create_api_objects()
{
    CGPUInstanceDescriptor instance_desc{};
    instance_desc.backend                     = CGPU_BACKEND_D3D12;
    instance_desc.enable_debug_layer          = true;
    instance_desc.enable_gpu_based_validation = false;
    instance_desc.enable_set_name             = true;

    _instance = cgpu_create_instance(&instance_desc);

    uint32_t adapters_count = 0;
    cgpu_enum_adapters(_instance, CGPU_NULLPTR, &adapters_count);
    CGPUAdapterId adapters[64];
    cgpu_enum_adapters(_instance, adapters, &adapters_count);
    _adapter = adapters[0];

    CGPUQueueGroupDescriptor queue_group_desc = {};
    queue_group_desc.queue_type               = CGPU_QUEUE_TYPE_GRAPHICS;
    queue_group_desc.queue_count              = 1;
    CGPUDeviceDescriptor device_desc          = {};
    device_desc.queue_groups                  = &queue_group_desc;
    device_desc.queue_group_count             = 1;
    _device                                   = cgpu_create_device(_adapter, &device_desc);
    _gfx_queue                                = cgpu_get_queue(_device, CGPU_QUEUE_TYPE_GRAPHICS, 0);
}

void Renderer::create_render_pipeline()
{
    uint32_t *vs_bytes, vs_length;
    uint32_t *fs_bytes, fs_length;
    read_shader_bytes(SKR_UTF8("AnimDebug/vertex_shader"), &vs_bytes, &vs_length, CGPU_BACKEND_D3D12);
    read_shader_bytes(SKR_UTF8("AnimDebug/fragment_shader"), &fs_bytes, &fs_length, CGPU_BACKEND_D3D12);
    CGPUShaderLibraryDescriptor vs_desc = {};
    vs_desc.stage                       = CGPU_SHADER_STAGE_VERT;
    vs_desc.name                        = SKR_UTF8("VertexShaderLibrary");
    vs_desc.code                        = vs_bytes;
    vs_desc.code_size                   = vs_length;
    CGPUShaderLibraryDescriptor ps_desc = {};
    ps_desc.name                        = SKR_UTF8("FragmentShaderLibrary");
    ps_desc.stage                       = CGPU_SHADER_STAGE_FRAG;
    ps_desc.code                        = fs_bytes;
    ps_desc.code_size                   = fs_length;
    CGPUShaderLibraryId vertex_shader   = cgpu_create_shader_library(_device, &vs_desc);
    CGPUShaderLibraryId fragment_shader = cgpu_create_shader_library(_device, &ps_desc);
    free(vs_bytes);
    free(fs_bytes);
    CGPUShaderEntryDescriptor ppl_shaders[2];
    ppl_shaders[0].stage                 = CGPU_SHADER_STAGE_VERT;
    ppl_shaders[0].entry                 = SKR_UTF8("main");
    ppl_shaders[0].library               = vertex_shader;
    ppl_shaders[1].stage                 = CGPU_SHADER_STAGE_FRAG;
    ppl_shaders[1].entry                 = SKR_UTF8("main");
    ppl_shaders[1].library               = fragment_shader;
    CGPURootSignatureDescriptor rs_desc  = {};
    rs_desc.shaders                      = ppl_shaders;
    rs_desc.shader_count                 = 2;
    _root_sig                            = cgpu_create_root_signature(_device, &rs_desc);
    CGPUVertexLayout vertex_layout       = {};
    vertex_layout.attribute_count        = 0;
    CGPURenderPipelineDescriptor rp_desc = {};
    rp_desc.root_signature               = _root_sig;
    rp_desc.prim_topology                = CGPU_PRIM_TOPO_TRI_LIST;
    rp_desc.vertex_layout                = &vertex_layout;
    rp_desc.vertex_shader                = &ppl_shaders[0];
    rp_desc.fragment_shader              = &ppl_shaders[1];
    rp_desc.render_target_count          = 1;
    auto backend_format                  = (ECGPUFormat)_swapchain->back_buffers[0]->info->format;
    rp_desc.color_formats                = &backend_format;
    _pipeline                            = cgpu_create_render_pipeline(_device, &rp_desc);
    cgpu_free_shader_library(vertex_shader);
    cgpu_free_shader_library(fragment_shader);
}

void Renderer::create_resources() {}
void Renderer::finalize() {}
void Renderer::destroy()
{
    cgpu_free_queue(_gfx_queue);
    cgpu_free_device(_device);
    cgpu_free_instance(_instance);
}
} // namespace animd