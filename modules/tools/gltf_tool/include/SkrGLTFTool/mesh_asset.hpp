#pragma once
#include "SkrToolCore/asset/importer.hpp"
#include "SkrToolCore/asset/cooker.hpp"
#include "SkrContainersDef/string.hpp"
#include "SkrContainersDef/vector.hpp"
#ifndef __meta__
    #include "SkrGLTFTool/mesh_asset.generated.h" // IWYU pragma: export
#endif

namespace skd::asset
{
sreflect_struct(
    guid = "D72E2056-3C12-402A-A8B8-148CB8EAB922" serde = @json)
GLTFTOOL_API GltfMeshImporter final : public Importer
{
    String assetPath;

    Vector<skr_guid_t> materials;

    bool invariant_vertices = false;
    bool install_to_ram = false;
    bool install_to_vram = true;

    void* Import(skr::io::IRAMService*, CookContext * context) override;
    void Destroy(void* resource) override;
};

sreflect_struct(guid = "5a378356-7bfa-461a-9f96-4bbbd2e95368")
GLTFTOOL_API MeshCooker final : public Cooker
{
    bool Cook(CookContext * ctx) override;
    uint32_t Version() override;
};
} // namespace skd::asset