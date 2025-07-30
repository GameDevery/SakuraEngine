#pragma once
#include "SkrGraphics/api.h"
#include "SkrCore/platform/vfs.h"
#include "SkrRenderer/render_device.h"
#include "SkrContainersDef/string.hpp"
#include "SkrRenderer/primitive_draw.h"

namespace utils
{

inline uint32_t* read_shader_bytes(skr::RendererDevice* render_device, skr_vfs_t* resource_vfs, const char8_t* name, uint32_t* out_length)
{
    const auto cgpu_device = render_device->get_cgpu_device();
    const auto backend = cgpu_device->adapter->instance->backend;
    skr::String shader_name = name;
    shader_name.append(backend == ::CGPU_BACKEND_D3D12 ? u8".dxil" : u8".spv");
    auto shader_file = skr_vfs_fopen(resource_vfs, shader_name.c_str(), SKR_FM_READ_BINARY, SKR_FILE_CREATION_OPEN_EXISTING);
    const uint32_t shader_length = (uint32_t)skr_vfs_fsize(shader_file);
    auto shader_bytes = (uint32_t*)sakura_malloc(shader_length);
    skr_vfs_fread(shader_file, shader_bytes, 0, shader_length);
    skr_vfs_fclose(shader_file);
    if (out_length) *out_length = shader_length;
    return shader_bytes;
}

inline CGPUShaderLibraryId create_shader_library(skr::RendererDevice* render_device, skr_vfs_t* resource_vfs, const char8_t* name, ECGPUShaderStage stage)
{
    const auto cgpu_device = render_device->get_cgpu_device();
    uint32_t shader_length = 0;
    uint32_t* shader_bytes = read_shader_bytes(render_device, resource_vfs, name, &shader_length);
    CGPUShaderLibraryDescriptor shader_desc = {};
    shader_desc.name = name;
    shader_desc.code = shader_bytes;
    shader_desc.code_size = shader_length;
    CGPUShaderLibraryId shader = cgpu_create_shader_library(cgpu_device, &shader_desc);
    sakura_free(shader_bytes);
    return shader;
}

struct SCENE_RENDERER_API DummyScene
{
    DummyScene();
    void init(skr::RendererDevice* render_device);
    skr::span<skr::renderer::PrimitiveCommand> get_primitive_commands();
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
    CGPUBufferId vertex_buffer = nullptr;
    CGPUBufferId index_buffer = nullptr;
    skr::Vector<skr_vertex_buffer_view_t> vbvs;
    skr_index_buffer_view_t ibv;
    skr::Vector<skr::renderer::PrimitiveCommand> primitive_commands;
};

} // namespace utils
