#pragma once
#include "SkrBase/math/gen/gen_math_c_decl.hpp"
#include "SkrGraphics/api.h"
#include "SkrCore/platform/vfs.h"
#include "SkrRenderer/render_device.h"
#include "SkrContainersDef/string.hpp"
#include "SkrRenderer/primitive_draw.h"
#include "SkrBase/math.h"

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

class SCENE_RENDERER_API SimpleMesh
{
protected:
    skr::Vector<skr_float3_t> c_positions;
    skr::Vector<skr_float2_t> c_uvs;
    skr::Vector<skr_float3_t> c_normals;
    skr::Vector<uint32_t> c_indices;

    CGPUBufferId vertex_buffer = nullptr;
    CGPUBufferId index_buffer = nullptr;
    skr::Vector<skr_vertex_buffer_view_t> vbvs;
    skr_index_buffer_view_t ibv;
    skr::Vector<skr::renderer::PrimitiveCommand> primitive_commands;

public:
    virtual void init() SKR_NOEXCEPT = 0;                                                                       // init instance, allocate CPU resources
    virtual void destroy() SKR_NOEXCEPT = 0;                                                                    // destroy all CPU and GPU resources
    void generate_render_mesh(skr::RendererDevice* render_device, skr_render_mesh_id render_mesh) SKR_NOEXCEPT; // generate render mesh, allocate GPU resources
};

class SCENE_RENDERER_API TriangleMesh : public SimpleMesh
{
public:
    virtual void init() SKR_NOEXCEPT override;
    virtual void destroy() SKR_NOEXCEPT override;
};

class SCENE_RENDERER_API CubeMesh : public SimpleMesh
{
    float size = 1.0f;

public:
    virtual void init() SKR_NOEXCEPT override;
    virtual void destroy() SKR_NOEXCEPT override;
    void set_size(float new_size) { size = new_size; }
    float get_size() const { return size; }
};

// a tiled grid 2D mesh
class SCENE_RENDERER_API Grid2DMesh : public SimpleMesh
{
    int num_tiles_width = 2;
    int num_tiles_height = 2;
    float tile_size_width = 1.0f;
    float tile_size_height = 1.0f;

public:
    virtual void init() SKR_NOEXCEPT override;
    virtual void destroy() SKR_NOEXCEPT override;

    // get and set methods for grid properties
    int get_num_tiles_width() const { return num_tiles_width; }
    int get_num_tiles_height() const { return num_tiles_height; }
    float get_tile_size_width() const { return tile_size_width; }
    float get_tile_size_height() const { return tile_size_height; }
    void set_num_tiles_width(int width) { num_tiles_width = width; }
    void set_num_tiles_height(int height) { num_tiles_height = height; }
    void set_tile_size_width(float size) { tile_size_width = size; }
    void set_tile_size_height(float size) { tile_size_height = size; }
};

} // namespace utils
