#include "helper.hpp"
#include "SkrBase/misc/make_zeroed.hpp"
#include "SkrRenderer/render_mesh.h"

namespace utils
{

void SimpleMesh::generate_render_mesh(skr::RendererDevice* render_device, skr_render_mesh_id render_mesh) SKR_NOEXCEPT
{
    // generate buffer, ub, ib, ubv, ibv
    auto cgpu_device = render_device->get_cgpu_device();
    auto positions_size = c_positions.size() * sizeof(skr_float3_t);
    auto uvs_size = c_uvs.size() * sizeof(skr_float2_t);
    auto normals_size = c_normals.size() * sizeof(skr_float3_t);
    auto vertex_size = positions_size + 2 * uvs_size + normals_size;
    SKR_LOG_INFO(u8"vertex size with positions %zu, uvs %zu, normals %zu is %zu", positions_size, uvs_size, normals_size, vertex_size);

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
        memcpy(vertex_buffer->info->cpu_mapped_address, c_positions.data(), positions_size);
        memcpy((uint8_t*)vertex_buffer->info->cpu_mapped_address + positions_size, c_uvs.data(), uvs_size);
        memcpy((uint8_t*)vertex_buffer->info->cpu_mapped_address + positions_size + uvs_size, c_uvs.data(), uvs_size);
        memcpy((uint8_t*)vertex_buffer->info->cpu_mapped_address + positions_size + 2 * uvs_size, c_normals.data(), normals_size);
    }

    auto index_size = c_indices.size() * sizeof(uint32_t);
    SKR_LOG_INFO(u8"index size is %zu", index_size);

    auto ib_desc = make_zeroed<CGPUBufferDescriptor>();
    ib_desc.name = u8"scene-renderer-indices";
    ib_desc.size = index_size;
    ib_desc.memory_usage = CGPU_MEM_USAGE_CPU_TO_GPU;
    ib_desc.descriptors = CGPU_RESOURCE_TYPE_INDEX_BUFFER;
    ib_desc.flags = CGPU_BCF_PERSISTENT_MAP_BIT;
    ib_desc.prefer_on_device = true;
    index_buffer = cgpu_create_buffer(cgpu_device, &ib_desc);

    {
        memcpy(index_buffer->info->cpu_mapped_address, c_indices.data(), index_size);
    }
    // construct triangle vbvs and ibv
    skr_vertex_buffer_view_t vbv = {};
    vbv.buffer = vertex_buffer;
    vbv.stride = sizeof(skr_float3_t);
    vbv.offset = 0;
    vbvs.push_back(vbv);

    vbv.buffer = vertex_buffer;
    vbv.stride = sizeof(skr_float2_t);
    vbv.offset = positions_size;
    vbvs.push_back(vbv);

    vbv.buffer = vertex_buffer;
    vbv.stride = sizeof(skr_float2_t);
    vbv.offset = positions_size + uvs_size;
    vbvs.push_back(vbv);

    vbv.buffer = vertex_buffer;
    vbv.stride = sizeof(skr_float3_t);
    vbv.offset = positions_size + 2 * uvs_size;
    vbvs.push_back(vbv);

    SKR_LOG_INFO(u8"vbvs size: %d", vbvs.size());

    ibv.buffer = index_buffer;
    ibv.stride = sizeof(uint32_t);
    ibv.offset = 0;
    ibv.index_count = (uint32_t)c_indices.size();
    ibv.first_index = 0;

    // generate primitive commands
    if (primitive_commands.is_empty())
    {
        auto& cmd = primitive_commands.emplace().ref();
        cmd.vbvs = { vbvs.data(), (uint32_t)vbvs.size() };
        cmd.ibv = &ibv;
    }

    render_mesh->index_buffer_views.push_back(ibv);
    render_mesh->vertex_buffer_views = vbvs;
    render_mesh->primitive_commands = primitive_commands;
}

void TriangleMesh::init() SKR_NOEXCEPT
{
    c_positions.push_back({ -1.0f, -0.5f, 0.0f });
    c_positions.push_back({ 1.0f, -1.0f, 0.0f });
    c_positions.push_back({ 0.0f, 1.0f, 0.0f });

    c_uvs.push_back({ 0.0f, 1.0f });
    c_uvs.push_back({ 1.0f, 1.0f });
    c_uvs.push_back({ 0.5f, 0.0f });

    c_normals.push_back({ 0.0f, 0.0f, 1.0f });
    c_normals.push_back({ 0.0f, 0.0f, 1.0f });
    c_normals.push_back({ 0.0f, 0.0f, 1.0f });

    c_indices.append({ 0, 1, 2 });
}

void TriangleMesh::destroy() SKR_NOEXCEPT
{
    if (vertex_buffer)
    {
        cgpu_free_buffer(vertex_buffer);
        vertex_buffer = nullptr;
    }
    if (index_buffer)
    {
        cgpu_free_buffer(index_buffer);
        index_buffer = nullptr;
    }
    c_positions.clear();
    c_uvs.clear();
    c_normals.clear();
    c_indices.clear();
}

void CubeMesh::init() SKR_NOEXCEPT
{
    float half_size = size * 0.5f;

    // 8 vertices of a cube
    c_positions.push_back({ -half_size, -half_size, -half_size }); // 0: left-bottom-back
    c_positions.push_back({ half_size, -half_size, -half_size });  // 1: right-bottom-back
    c_positions.push_back({ half_size, half_size, -half_size });   // 2: right-top-back
    c_positions.push_back({ -half_size, half_size, -half_size });  // 3: left-top-back
    c_positions.push_back({ -half_size, -half_size, half_size });  // 4: left-bottom-front
    c_positions.push_back({ half_size, -half_size, half_size });   // 5: right-bottom-front
    c_positions.push_back({ half_size, half_size, half_size });    // 6: right-top-front
    c_positions.push_back({ -half_size, half_size, half_size });   // 7: left-top-front

    // UV coordinates for each vertex
    c_uvs.push_back({ 0.0f, 0.0f });
    c_uvs.push_back({ 1.0f, 0.0f });
    c_uvs.push_back({ 1.0f, 1.0f });
    c_uvs.push_back({ 0.0f, 1.0f });
    c_uvs.push_back({ 0.0f, 0.0f });
    c_uvs.push_back({ 1.0f, 0.0f });
    c_uvs.push_back({ 1.0f, 1.0f });
    c_uvs.push_back({ 0.0f, 1.0f });

    // Normals for each vertex (cube has 6 faces, but we'll use averaged normals)
    c_normals.push_back({ -0.577f, -0.577f, -0.577f }); // normalized diagonal
    c_normals.push_back({ 0.577f, -0.577f, -0.577f });
    c_normals.push_back({ 0.577f, 0.577f, -0.577f });
    c_normals.push_back({ -0.577f, 0.577f, -0.577f });
    c_normals.push_back({ -0.577f, -0.577f, 0.577f });
    c_normals.push_back({ 0.577f, -0.577f, 0.577f });
    c_normals.push_back({ 0.577f, 0.577f, 0.577f });
    c_normals.push_back({ -0.577f, 0.577f, 0.577f });

    // 12 triangles (36 indices) for cube faces
    // Back face
    c_indices.append({ 0, 1, 2, 0, 2, 3 });
    c_indices.append({ 4, 6, 5, 4, 7, 6 });
    c_indices.append({ 0, 3, 7, 0, 7, 4 });
    c_indices.append({ 1, 5, 6, 1, 6, 2 });
    c_indices.append({ 0, 4, 5, 0, 5, 1 });
    c_indices.append({ 3, 2, 6, 3, 6, 7 });
}

void CubeMesh::destroy() SKR_NOEXCEPT
{
    if (vertex_buffer)
    {
        cgpu_free_buffer(vertex_buffer);
        vertex_buffer = nullptr;
    }
    if (index_buffer)
    {
        cgpu_free_buffer(index_buffer);
        index_buffer = nullptr;
    }
    c_positions.clear();
    c_uvs.clear();
    c_normals.clear();
    c_indices.clear();
    vbvs.clear();
    primitive_commands.clear();
}

void Grid2DMesh::init() SKR_NOEXCEPT
{
    // Calculate total vertices and indices
    int vertices_width = num_tiles_width + 1;
    int vertices_height = num_tiles_height + 1;
    // Generate vertices
    for (int y = 0; y < vertices_height; ++y)
    {
        for (int x = 0; x < vertices_width; ++x)
        {
            // Position
            float pos_x = x * tile_size_width - (num_tiles_width * tile_size_width * 0.5f);
            float pos_z = y * tile_size_height - (num_tiles_height * tile_size_height * 0.5f);
            c_positions.push_back({ pos_x, 0.0f, pos_z });

            // UV coordinates
            float u = (float)x / (float)num_tiles_width;
            float v = (float)y / (float)num_tiles_height;
            c_uvs.push_back({ u, v });

            // Normal (pointing up for a flat grid)
            c_normals.push_back({ 0.0f, 1.0f, 0.0f });
        }
    }

    // Generate indices (two triangles per tile)
    for (int y = 0; y < num_tiles_height; ++y)
    {
        for (int x = 0; x < num_tiles_width; ++x)
        {
            int bottom_left = y * vertices_width + x;
            int bottom_right = bottom_left + 1;
            int top_left = bottom_left + vertices_width;
            int top_right = top_left + 1;

            // First triangle (bottom-left, bottom-right, top-left)
            c_indices.append({ static_cast<uint32_t>(bottom_left),
                static_cast<uint32_t>(bottom_right),
                static_cast<uint32_t>(top_left) });

            // Second triangle (bottom-right, top-right, top-left)
            c_indices.append({ static_cast<uint32_t>(bottom_right),
                static_cast<uint32_t>(top_right),
                static_cast<uint32_t>(top_left) });
        }
    }
}

void Grid2DMesh::destroy() SKR_NOEXCEPT
{
    if (vertex_buffer)
    {
        cgpu_free_buffer(vertex_buffer);
        vertex_buffer = nullptr;
    }
    if (index_buffer)
    {
        cgpu_free_buffer(index_buffer);
        index_buffer = nullptr;
    }
    c_positions.clear();
    c_uvs.clear();
    c_normals.clear();
    c_indices.clear();
    vbvs.clear();
    primitive_commands.clear();
}

} // namespace utils