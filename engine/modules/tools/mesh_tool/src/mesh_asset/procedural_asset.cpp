#include "SkrBase/misc/defer.hpp"
#include "SkrCore/log.hpp"
#include "SkrTask/parallel_for.hpp"
#include "SkrContainers/stl_vector.hpp"
#include "SkrToolCore/cook_system/cook_system.hpp"
#include "SkrToolCore/project/project.hpp"
#include "SkrMeshTool/mesh_asset.hpp"
#include "SkrMeshTool/mesh_processing.hpp"
#include "MeshOpt/meshoptimizer.h"

#include "SkrProfile/profile.h"
#include "SkrRTTR/type.hpp"
#include "SkrMeshCore/mesh_processing.hpp"
#include "SkrContainersDef/stl_string.hpp"
#include "SkrGraphics/api.h"

namespace skd::asset
{

void* ProceduralMeshImporter::Import(skr::io::IRAMService* ioService, CookContext* context)
{
    skr::RTTRType* type = get_type_from_guid(built_in_mesh_tid);
    if (!type)
    {
        SKR_LOG_FATAL(u8"BuiltinMeshImporter: Failed to find type for guid %s", built_in_mesh_tid);
        return nullptr;
    }
    void* data = sakura_malloc_aligned(type->size(), type->alignment());
    type->find_default_ctor().invoke(data);
    return (void*)data;
}

void ProceduralMeshImporter::Destroy(void* resource)
{
    skr::RTTRType* type = get_type_from_guid(built_in_mesh_tid);
    sakura_free_aligned((ProceduralMesh*)resource, type->alignment());
    return;
}

void SimpleTriangleMesh::configure(const ProceduralArgsBase* args)
{
    if (args)
    {
        auto pArgs = static_cast<const Args*>(args);
    }
}

void SimpleTriangleMesh::generate_resource(skr::MeshResource& out_resource, skr::Vector<skr::Vector<uint8_t>>& out_bins, skr_guid_t shuffle_layout_id)
{
    // TODO: directly construct on bins, no copy
    skr::Vector<uint8_t> buffer0 = {};
    out_resource.name = u8"BuiltInSimpleTriangleMesh";
    auto& mesh_section = out_resource.sections.add_default().ref();
    mesh_section.translation = { 0.0f, 0.0f, 0.0f };
    mesh_section.scale = { 1.0f, 1.0f, 1.0f };
    mesh_section.rotation = { 0.0f, 0.0f, 0.0f, 1.0f };

    CGPUVertexLayout shuffle_layout = {};
    const char* shuffle_layout_name = nullptr;
    if (!shuffle_layout_id.is_zero())
    {
        shuffle_layout_name = skr_mesh_resource_query_vertex_layout(shuffle_layout_id, &shuffle_layout);
    }
    uint32_t indices[] = { 0, 1, 2 };
    // clang-format off
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
        0.5f, 0.5f, 0.0f,
    };
    float normals[] = {
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
    };
    float texcoord[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
    };

    // clang-format on
    SRawMesh raw_mesh = {};
    raw_mesh.primitives.reserve(1);
    auto& raw_prim = raw_mesh.primitives.add_default().ref();
    // fill indices
    {
        raw_prim.index_stream.buffer_view = skr::span<const uint8_t>((const uint8_t*)indices, sizeof(indices));
        raw_prim.index_stream.offset = 0;
        raw_prim.index_stream.count = 3;
        raw_prim.index_stream.stride = sizeof(uint32_t);
    }
    // fill vertex stream
    {
        raw_prim.vertex_streams.reserve(4);

        auto& vertex_stream = raw_prim.vertex_streams.add_default().ref();
        vertex_stream.buffer_view = skr::span<const uint8_t>((const uint8_t*)vertices, sizeof(vertices));
        vertex_stream.offset = 0;
        vertex_stream.count = 3;
        vertex_stream.stride = sizeof(float) * 3;
        vertex_stream.type = ERawVertexStreamType::POSITION;

        auto& normal_stream = raw_prim.vertex_streams.add_default().ref();
        normal_stream.buffer_view = skr::span<const uint8_t>((const uint8_t*)normals, sizeof(normals));
        normal_stream.offset = 0;
        normal_stream.count = 3;
        normal_stream.stride = sizeof(float) * 3;
        normal_stream.type = ERawVertexStreamType::NORMAL;

        auto& texcoord_stream = raw_prim.vertex_streams.add_default().ref();
        texcoord_stream.buffer_view = skr::span<const uint8_t>((const uint8_t*)texcoord, sizeof(texcoord));
        texcoord_stream.offset = 0;
        texcoord_stream.count = 3;
        texcoord_stream.stride = sizeof(float) * 2;
        texcoord_stream.type = ERawVertexStreamType::TEXCOORD;
    }

    skr::Vector<MeshPrimitive> new_primitives;
    EmplaceAllRawMeshIndices(&raw_mesh, buffer0, new_primitives);
    EmplaceAllRawMeshVertices(&raw_mesh, shuffle_layout_name ? &shuffle_layout : nullptr, buffer0, new_primitives);
    SKR_ASSERT(new_primitives.size() == 1 && "SimpleTriangleMesh should have only one primitive");
    auto& prim0 = new_primitives[0];
    prim0.vertex_layout = shuffle_layout_id;
    prim0.material_index = 0; // no material
    mesh_section.primitive_indices.add(out_resource.primitives.size());
    out_resource.primitives.reserve(1);
    out_resource.primitives += new_primitives;

    {
        // record buffer bins
        auto& out_buffer0 = out_resource.bins.add_default().ref();
        out_buffer0.index = 0;
        out_buffer0.byte_length = buffer0.size();
        out_buffer0.used_with_index = true;
        out_buffer0.used_with_vertex = true;
    }

    out_bins.add(buffer0);
}

void SimpleCubeMesh::configure(const ProceduralArgsBase* args)
{
    if (args)
    {
        auto pArgs = static_cast<const Args*>(args);
    }
}

void SimpleCubeMesh::generate_resource(skr::MeshResource& out_resource, skr::Vector<skr::Vector<uint8_t>>& out_bins, skr_guid_t shuffle_layout_id)
{
    skr::Vector<uint8_t> buffer0 = {};
    out_resource.name = u8"BuiltInSimpleCubeMesh";
    auto& mesh_section = out_resource.sections.add_default().ref();
    mesh_section.translation = { 0.0f, 0.0f, 0.0f };
    mesh_section.scale = { 1.0f, 1.0f, 1.0f };
    mesh_section.rotation = { 0.0f, 0.0f, 0.0f, 1.0f };

    CGPUVertexLayout shuffle_layout = {};
    const char* shuffle_layout_name = nullptr;
    if (!shuffle_layout_id.is_zero())
    {
        shuffle_layout_name = skr_mesh_resource_query_vertex_layout(shuffle_layout_id, &shuffle_layout);
    }
    // clang-format off
    uint32_t indices[] = { 0, 1, 2, 0, 2, 3,
                            4, 6, 5, 4, 7, 6 ,
                           0, 3, 7, 0, 7, 4 ,
                           1, 5, 6, 1, 6, 2 ,
                           0, 4, 5, 0, 5, 1,
                           3, 2, 6, 3, 6, 7 };
    // clang-format off
    float vertices[] = {
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
    };
    constexpr int index_count = 36;
    constexpr int vertex_count = 8;
    float texcoord[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
    };

    // clang-format on
    SRawMesh raw_mesh = {};
    raw_mesh.primitives.reserve(1);
    auto& raw_prim = raw_mesh.primitives.add_default().ref();
    // fill indices
    {
        raw_prim.index_stream.buffer_view = skr::span<const uint8_t>((const uint8_t*)indices, sizeof(indices));
        raw_prim.index_stream.offset = 0;
        raw_prim.index_stream.count = index_count;
        raw_prim.index_stream.stride = sizeof(uint32_t);
    }
    // fill vertex stream
    {
        raw_prim.vertex_streams.reserve(4);

        auto& vertex_stream = raw_prim.vertex_streams.add_default().ref();
        vertex_stream.buffer_view = skr::span<const uint8_t>((const uint8_t*)vertices, sizeof(vertices));
        vertex_stream.offset = 0;
        vertex_stream.count = vertex_count;
        vertex_stream.stride = sizeof(float) * 3;
        vertex_stream.type = ERawVertexStreamType::POSITION;

        auto& texcoord_stream = raw_prim.vertex_streams.add_default().ref();
        texcoord_stream.buffer_view = skr::span<const uint8_t>((const uint8_t*)texcoord, sizeof(texcoord));
        texcoord_stream.offset = 0;
        texcoord_stream.count = vertex_count;
        texcoord_stream.stride = sizeof(float) * 2;
        texcoord_stream.type = ERawVertexStreamType::TEXCOORD;
    }

    skr::Vector<MeshPrimitive> new_primitives;
    EmplaceAllRawMeshIndices(&raw_mesh, buffer0, new_primitives);
    EmplaceAllRawMeshVertices(&raw_mesh, shuffle_layout_name ? &shuffle_layout : nullptr, buffer0, new_primitives);
    SKR_ASSERT(new_primitives.size() == 1 && "SimpleTriangleMesh should have only one primitive");
    auto& prim0 = new_primitives[0];
    prim0.vertex_layout = shuffle_layout_id;
    prim0.material_index = 0; // no material
    mesh_section.primitive_indices.add(out_resource.primitives.size());
    out_resource.primitives.reserve(1);
    out_resource.primitives += new_primitives;

    {
        // record buffer bins
        auto& out_buffer0 = out_resource.bins.add_default().ref();
        out_buffer0.index = 0;
        out_buffer0.byte_length = buffer0.size();
        out_buffer0.used_with_index = true;
        out_buffer0.used_with_vertex = true;
    }

    out_bins.add(buffer0);
}

void SimpleGridMesh::configure(const ProceduralArgsBase* args)
{
    if (args)
    {
        auto pArgs = static_cast<const Args*>(args);
    }
}
void SimpleGridMesh::generate_resource(skr::MeshResource& out_resource, skr::Vector<skr::Vector<uint8_t>>& out_bins, skr_guid_t shuffle_layout_id)
{
}

} // namespace skd::asset
