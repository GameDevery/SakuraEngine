#pragma once
#include "SkrToolCore/cook_system/importer.hpp"
#include "SkrToolCore/cook_system/cooker.hpp"
#include "SkrContainersDef/string.hpp"
#include "SkrRenderer/resources/mesh_resource.h"
#include "SkrMeshCore/mesh_processing.hpp"
#ifndef __meta__
    #include "SkrMeshTool/mesh_asset.generated.h" // IWYU pragma: export
#endif

struct cgltf_data;

namespace skd::asset
{

sreflect_struct(
    guid = "0198d1f8-cc5f-7012-adce-23b02354c964";
    serde = @json)
MESH_TOOL_API ProceduralMesh
{
    virtual void configure(const MeshAsset* args) {}
    virtual void generate_resource(skr::MeshResource & out_resource, skr::Vector<skr::Vector<uint8_t>> & out_bins, skr_guid_t shuffle_layout_id) = 0;
    virtual ~ProceduralMesh() = default;
};

sreflect_struct(
    guid = "0198d1f8-e193-7229-9953-f83da95f211b";
    serde = @json)
MESH_TOOL_API SimpleTriangleMesh final : public ProceduralMesh
{
    virtual void generate_resource(skr::MeshResource & out_resource, skr::Vector<skr::Vector<uint8_t>> & out_bins, skr_guid_t shuffle_layout_id) override;
    virtual ~SimpleTriangleMesh() = default;
};

sreflect_struct(
    guid = "0198d1f8-fb87-77ad-8497-c9e670262ae3";
    serde = @json)
MESH_TOOL_API SimpleCubeMesh final : public ProceduralMesh
{
    virtual void generate_resource(skr::MeshResource & out_resource, skr::Vector<skr::Vector<uint8_t>> & out_bins, skr_guid_t shuffle_layout_id) override;
    virtual ~SimpleCubeMesh() = default;
};

sreflect_struct(
    guid = "0198d227-06aa-7297-be1d-4bface06db38" serde = @json)
SimpleGridMeshAsset final : public MeshAsset
{
    uint32_t x_segments = 1;
    uint32_t y_segments = 1;
    float x_size = 1.0f;
    float y_size = 1.0f;
};

sreflect_struct(
    guid = "0198d206-193f-71ad-b7f5-bd0b82480b9a";
    serde = @json)
MESH_TOOL_API SimpleGridMesh final : public ProceduralMesh
{
    virtual void configure(const MeshAsset* args) override;
    void generate_resource(skr::MeshResource & out_resource, skr::Vector<skr::Vector<uint8_t>> & out_bins, skr_guid_t shuffle_layout_id) override;
    virtual ~SimpleGridMesh() = default;
    uint32_t x_segments = 1;
    uint32_t y_segments = 1;
    float x_size = 1.0f;
    float y_size = 1.0f;
};

sreflect_struct(
    guid = "0198d1ff-f92f-7068-9421-5221298cca87";
    serde = @json)
MESH_TOOL_API ProceduralMeshImporter final : public Importer
{
    skr_guid_t built_in_mesh_tid;

    void* Import(skr::io::IRAMService*, CookContext * context) override;
    void Destroy(void* resource) override;
};

sreflect_struct(guid = "D72E2056-3C12-402A-A8B8-148CB8EAB922" serde = @json)
MESH_TOOL_API GltfMeshImporter final : public Importer
{
public:
    struct ImportData
    {
        cgltf_data* gltf_data;
        skr::Vector<skr::GUID> import_materials;
        skr::Vector<skr::GUID> import_textures;
    };

    String assetPath;
    bool invariant_vertices = false;
    bool import_all_materials = false;

    void* Import(skr::io::IRAMService*, CookContext * context) override;
    void Destroy(void* resource) override;

private:
    void ImportMaterials(ImportData* data, CookContext* context);
};

sreflect_struct(guid = "5a378356-7bfa-461a-9f96-4bbbd2e95368")
MESH_TOOL_API MeshCooker final : public Cooker
{
    bool Cook(CookContext * ctx) override;
    uint32_t Version() override;
};
} // namespace skd::asset