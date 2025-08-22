#include "SkrMeshCore/builtin_mesh.hpp"
#include "SkrMeshCore/mesh_processing.hpp"
#include "SkrContainersDef/stl_string.hpp"

namespace skd::asset
{
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
        // vertex_stream.type = ERawVertexStreamType::POSITION;
        vertex_stream.type = ERawVertexStreamType::NORMAL;

        auto& normal_stream = raw_prim.vertex_streams.add_default().ref();
        normal_stream.buffer_view = skr::span<const uint8_t>((const uint8_t*)normals, sizeof(normals));
        normal_stream.offset = 0;
        normal_stream.count = 3;
        normal_stream.stride = sizeof(float) * 3;
        // normal_stream.type = ERawVertexStreamType::NORMAL;
        normal_stream.type = ERawVertexStreamType::TANGENT;

        auto& tangent_stream = raw_prim.vertex_streams.add_default().ref();

        auto& texcoord_stream = raw_prim.vertex_streams.add_default().ref();
        texcoord_stream.buffer_view = skr::span<const uint8_t>((const uint8_t*)texcoord, sizeof(texcoord));
        texcoord_stream.offset = 0;
        texcoord_stream.count = 3;
        texcoord_stream.stride = sizeof(float) * 2;
        // texcoord_stream.type = ERawVertexStreamType::TEXCOORD;
        texcoord_stream.type = ERawVertexStreamType::COLOR;
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

void SimpleCubeMesh::generate_resource(skr::MeshResource& out_resource, skr::Vector<skr::Vector<uint8_t>>& out_bins, skr_guid_t shuffle_layout_id)
{
}

} // namespace skd::asset