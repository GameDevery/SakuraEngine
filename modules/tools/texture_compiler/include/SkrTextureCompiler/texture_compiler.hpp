#pragma once
#include "SkrToolCore/asset/importer.hpp"
#include "SkrToolCore/asset/cooker.hpp"
#include "SkrContainersDef/string.hpp"
#ifndef __meta__
    #include "SkrTextureCompiler/texture_compiler.generated.h" // IWYU pragma: export
#endif

namespace skd::asset
{
sreflect_struct(
    guid = "a26c2436-9e5f-43c4-b4d7-e5373d353bae" serde = @json)
SKR_TEXTURE_COMPILER_API TextureImporter final : public Importer
{
    skr::String assetPath;

    void* Import(skr_io_ram_service_t*, CookContext * context) override;
    void Destroy(void* resource) override;
};

sreflect_struct(guid = "F9B45BF9-3767-4B40-B0B3-D4BBC228BCEC")
SKR_TEXTURE_COMPILER_API TextureCooker final : public Cooker
{
    bool Cook(CookContext * ctx) override;
    uint32_t Version() override;
};

} // namespace skd::asset