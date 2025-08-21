#include "SkrMeshCore/builtin_mesh.hpp"
#include "SkrMeshCore/mesh_processing.hpp"
#include "SkrContainersDef/stl_string.hpp"

namespace skd::asset
{
void SimpleTriangleMesh::generate_resource(skr::MeshResource& out_resource, skr::Vector<skr::Vector<uint8_t>>& out_bins, skr_guid_t shuffle_layout_id)
{
    skr::Vector<uint8_t> buffer0 = {};
    out_resource.name = u8"BuiltInSimpleTriangleMesh";
    auto& mesh_section = out_resource.sections.add_default().ref();
    mesh_section.translation = { 0.0f, 0.0f, 0.0f };
    mesh_section.scale = { 1.0f, 1.0f, 1.0f };
    mesh_section.rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
    auto& prim = out_resource.primitives.add_default().ref();
    prim.index_buffer = { 0, 0, 0, 0 };

    CGPUVertexLayout shuffle_layout = {};
    const char* shuffle_layout_name = nullptr;
    if (!shuffle_layout_id.is_zero())
    {
        shuffle_layout_name = skr_mesh_resource_query_vertex_layout(shuffle_layout_id, &shuffle_layout);
    }
    uint32_t indices[] = { 0, 1, 2, 2, 3, 0 };
    // clang-format off
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
        0.5f, 0.5f, 0.0f,
    };
    float texcoord0[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
    };
    float texcoord1[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
    };
    float normals[] = {
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
    };
    float tangents[] = {
        1.0f, 0.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 0.0f, 1.0f,
    };
    float3 colors[] = {
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f},
    };

    // clang-format on
    buffer0.append((const uint8_t*)indices, sizeof(indices));
    SRawMesh raw_mesh = {};
    raw_mesh.primitives.reserve(1);
    auto& raw_prim = raw_mesh.primitives.add_default().ref();
    // fill indices
    {
        raw_prim.index_stream.buffer_view = skr::span<const uint8_t>((const uint8_t*)indices, sizeof(indices));
        raw_prim.index_stream.offset = 0;
        raw_prim.index_stream.count = 6;
        raw_prim.index_stream.stride = sizeof(uint32_t);
    }
    // fill vertex stream
    {
        raw_prim.vertex_streams.reserve(1);
        auto& vertex_stream = raw_prim.vertex_streams.add_default().ref();
        vertex_stream.buffer_view = skr::span<const uint8_t>((const uint8_t*)vertices, sizeof(vertices));
        vertex_stream.offset = 0;
        vertex_stream.count = 3;
        vertex_stream.stride = sizeof(float) * 3;
        vertex_stream.type = ERawVertexStreamType::POSITION;

        auto& texcoord0_stream = raw_prim.vertex_streams.add_default().ref();
        texcoord0_stream.buffer_view = skr::span<const uint8_t>((const uint8_t*)texcoord0, sizeof(texcoord0));
        texcoord0_stream.offset = 0;
        texcoord0_stream.count = 3;
        texcoord0_stream.stride = sizeof(float) * 2;
        texcoord0_stream.type = ERawVertexStreamType::TEXCOORD;

        auto& texcoord1_stream = raw_prim.vertex_streams.add_default().ref();
        texcoord1_stream.buffer_view = skr::span<const uint8_t>((const uint8_t*)texcoord1, sizeof(texcoord1));
        texcoord1_stream.offset = 0;
        texcoord1_stream.count = 3;
        texcoord1_stream.stride = sizeof(float) * 2;
        texcoord1_stream.type = ERawVertexStreamType::TEXCOORD;

        auto& normal_stream = raw_prim.vertex_streams.add_default().ref();
        normal_stream.buffer_view = skr::span<const uint8_t>((const uint8_t*)normals, sizeof(normals));
        normal_stream.offset = 0;
        normal_stream.count = 3;
        normal_stream.stride = sizeof(float) * 3;
        normal_stream.type = ERawVertexStreamType::NORMAL;
    }
    skr::Vector<MeshPrimitive> new_primitives;
    EmplaceAllRawMeshIndices(&raw_mesh, buffer0, new_primitives);
    EmplaceAllRawMeshVertices(&raw_mesh, shuffle_layout_name ? &shuffle_layout : nullptr, buffer0, new_primitives);
    SKR_ASSERT(new_primitives.size() == 1 && "SimpleTriangleMesh should have only one primitive");
    auto& prim0 = new_primitives[0];
    prim0.vertex_layout = shuffle_layout_id;
    prim0.material_index = 0; // no material
    mesh_section.primitive_indices.add(out_resource.primitives.size());
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

void SimpleCubeMesh::generate_resource(skr::MeshResource& out_resource, skr::Vector<skr::Vector<uint8_t>>& out_bins, skr_guid_t shuffle_layout_id)
{
}

} // namespace skd::asset