#pragma once
#include "SkrToolCore/cook_system/importer.hpp"
#include "SkrToolCore/cook_system/cooker.hpp"
#include "SkrContainersDef/string.hpp"
#ifndef __meta__
    #include "SkrMeshTool/mesh_asset.generated.h" // IWYU pragma: export
#endif

namespace skd::asset
{
sreflect_struct(
    guid = "D72E2056-3C12-402A-A8B8-148CB8EAB922" serde = @json)
MESH_TOOL_API GltfMeshImporter final : public Importer
{
    String assetPath;
    bool invariant_vertices = false;

    void* Import(skr::io::IRAMService*, CookContext * context) override;
    void Destroy(void* resource) override;
};

sreflect_struct(guid = "5a378356-7bfa-461a-9f96-4bbbd2e95368")
MESH_TOOL_API MeshCooker final : public Cooker
{
    bool Cook(CookContext * ctx) override;
    uint32_t Version() override;
};
} // namespace skd::asset