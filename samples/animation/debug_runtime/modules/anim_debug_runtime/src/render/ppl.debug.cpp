#include "AnimDebugRuntime/renderer.h"
#include "common/utils.h"
namespace animd
{

void Renderer::create_debug_pipeline()
{
    uint32_t *vs_bytes, vs_length;
    uint32_t *fs_bytes, fs_length;
    read_shader_bytes(SKR_UTF8("AnimDebug/debug_vs"), &vs_bytes, &vs_length, _device->adapter->instance->backend);
    read_shader_bytes(SKR_UTF8("AnimDebug/debug_fs"), &fs_bytes, &fs_length, _device->adapter->instance->backend);
    CGPUShaderLibraryDescriptor vs_desc = {};
    vs_desc.name                        = SKR_UTF8("DebugVertexShader");
    vs_desc.code                        = vs_bytes;
    vs_desc.code_size                   = vs_length;
    CGPUShaderLibraryDescriptor ps_desc = {};
    ps_desc.name                        = SKR_UTF8("DebugFragmentShader");
    ps_desc.code                        = fs_bytes;
    ps_desc.code_size                   = fs_length;
    CGPUShaderLibraryId debug_vs        = cgpu_create_shader_library(_device, &vs_desc);
    CGPUShaderLibraryId debug_fs        = cgpu_create_shader_library(_device, &ps_desc);
    free(vs_bytes);
    free(fs_bytes);
    CGPUShaderEntryDescriptor ppl_shaders[2];
    ppl_shaders[0].stage                = CGPU_SHADER_STAGE_VERT;
    ppl_shaders[0].entry                = SKR_UTF8("main");
    ppl_shaders[0].library              = debug_vs;
    ppl_shaders[1].stage                = CGPU_SHADER_STAGE_FRAG;
    ppl_shaders[1].entry                = SKR_UTF8("main");
    ppl_shaders[1].library              = debug_fs;
    CGPURootSignatureDescriptor rs_desc = {};
    rs_desc.shaders                     = ppl_shaders;
    rs_desc.shader_count                = 2;
    rs_desc.push_constant_count         = 1;
    const char8_t* root_const_name      = SKR_UTF8("push_constants");
    rs_desc.push_constant_names         = &root_const_name;
    auto root_sig                       = cgpu_create_root_signature(_device, &rs_desc);

    CGPUVertexLayout vertex_layout = {};
    vertex_layout.attributes[0]    = { SKR_UTF8("POSITION"), 1, CGPU_FORMAT_R32G32B32_SFLOAT, 0, 0, sizeof(skr_float3_t), CGPU_INPUT_RATE_VERTEX };
    vertex_layout.attributes[1]    = { SKR_UTF8("COLOR"), 1, CGPU_FORMAT_R32G32B32_SFLOAT, 1, 0, sizeof(skr_float3_t), CGPU_INPUT_RATE_VERTEX };
    vertex_layout.attribute_count  = 2;

    const ECGPUFormat color_formats[1] = { CGPU_FORMAT_R8G8B8A8_UNORM };
    const ECGPUFormat ds_fmt           = CGPU_FORMAT_D32_SFLOAT;

    CGPURenderPipelineDescriptor rp_desc = {};
    rp_desc.root_signature               = root_sig;
    rp_desc.prim_topology                = CGPU_PRIM_TOPO_TRI_LIST;
    rp_desc.vertex_layout                = &vertex_layout;
    rp_desc.vertex_shader                = &ppl_shaders[0];
    rp_desc.fragment_shader              = &ppl_shaders[1];
    rp_desc.render_target_count          = sizeof(color_formats) / sizeof(ECGPUFormat);
    rp_desc.color_formats                = color_formats;
    rp_desc.depth_stencil_format         = ds_fmt;

    CGPURasterizerStateDescriptor raster_desc = {};
    raster_desc.cull_mode                     = ECGPUCullMode::CGPU_CULL_MODE_BACK;
    raster_desc.depth_bias                    = 0;
    raster_desc.fill_mode                     = CGPU_FILL_MODE_SOLID;
    raster_desc.front_face                    = CGPU_FRONT_FACE_CW;
    rp_desc.rasterizer_state                  = &raster_desc;

    CGPUDepthStateDescriptor ds_desc = {};
    ds_desc.depth_func               = CGPU_CMP_LEQUAL;
    ds_desc.depth_write              = true;
    ds_desc.depth_test               = true;
    rp_desc.depth_state              = &ds_desc;
    _debug_pipeline                  = cgpu_create_render_pipeline(_device, &rp_desc);
    cgpu_free_shader_library(debug_vs);
    cgpu_free_shader_library(debug_fs);
}

} // namespace animd