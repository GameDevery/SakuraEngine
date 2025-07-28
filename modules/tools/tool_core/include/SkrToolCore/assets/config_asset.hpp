#pragma once
#include "SkrToolCore/asset/importer.hpp"
#include "SkrToolCore/asset/cooker.hpp"
#ifndef __meta__
    #include "SkrToolCore/assets/config_asset.generated.h" // IWYU pragma: export
#endif

namespace skd::asset
{
sreflect_struct(
    guid = "D5970221-1A6B-42C4-B604-DA0559E048D6" serde = @json)
TOOL_CORE_API JsonConfigImporter final : public Importer
{
    skr::String assetPath;
    skr_guid_t configType;
    void* Import(skr::io::IRAMService*, CookContext * context) override;
    void Destroy(void* resource) override;
};

sreflect_struct(guid = "EC5275CA-E406-4051-9403-77517C421890")
TOOL_CORE_API ConfigCooker final : public Cooker
{
    bool Cook(CookContext * ctx) override;
    uint32_t Version() override;
};
} // namespace skd::asset