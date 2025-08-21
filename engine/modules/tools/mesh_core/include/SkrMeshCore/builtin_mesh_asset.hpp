#pragma once
#include "SkrToolCore/cook_system/importer.hpp"
#include "SkrToolCore/cook_system/cooker.hpp"
#include "SkrContainersDef/string.hpp"
#include "SkrContainersDef/map.hpp"

#ifndef __meta__
    #include "SkrMeshCore/builtin_mesh_asset.generated.h" // IWYU pragma: export
#endif

namespace skd::asset
{

sreflect_struct(
    guid = "0198cc2f-a789-7574-8047-dc332afeac83")
MESH_CORE_API BuiltinMeshImporter final : public Importer
{
    skr_guid_t built_in_mesh_tid;
    void* data = nullptr;
    void* Import(skr::io::IRAMService*, CookContext * context) override;
    void Destroy(void* resource) override;
};

sreflect_struct(
    guid = "0198cc39-e102-72f8-91e4-05a30ca0dc5b")
MESH_CORE_API BuiltinMeshCooker final : public Cooker
{
    bool Cook(CookContext * ctx) override;
    uint32_t Version() override;
};

} // namespace skd::asset