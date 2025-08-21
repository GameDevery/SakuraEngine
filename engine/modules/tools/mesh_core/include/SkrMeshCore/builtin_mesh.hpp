#pragma once
// The Simple Bulit-in Mesh Generator
#include "SkrBase/config.h"
#include "SkrRenderer/resources/mesh_resource.h"
#include "SkrContainersDef/vector.hpp"
#include "SkrGraphics/api.h"
#ifndef __meta__
    #include "SkrMeshCore/builtin_mesh.generated.h" // IWYU pragma: export
#endif

namespace skd::asset
{

sreflect_struct(
    guid = "0198cc6c-ded2-75ad-827e-ccd1903628c6";
    serde = @json)
MESH_CORE_API BuiltInMesh
{
    virtual void generate_resource(skr::MeshResource & out_resource, skr::Vector<skr::Vector<uint8_t>> & out_bins, skr_guid_t shuffle_layout_id) = 0;
};

sreflect_struct(
    guid = "0198cc6e-48e0-757e-9848-d5bbe4796a9e";
    serde = @json)
MESH_CORE_API SimpleTriangleMesh : public BuiltInMesh
{
    void generate_resource(skr::MeshResource & out_resource, skr::Vector<skr::Vector<uint8_t>> & out_bins, skr_guid_t shuffle_layout_id) override;
};

sreflect_struct(
    guid = "0198cc70-7f4c-7a5d-8e4e-1f3f3f0e2b6f";
    serde = @json)
MESH_CORE_API SimpleCubeMesh : public BuiltInMesh
{
    void generate_resource(skr::MeshResource & out_resource, skr::Vector<skr::Vector<uint8_t>> & out_bins, skr_guid_t shuffle_layout_id) override;
};

} // namespace skd::asset