#pragma once
#include "SkrBase/config.h"
#include "SkrToolCore/asset/importer.hpp"
#include "SkrRT/config.h"
#include "SkrRenderer/resources/shader_meta_resource.hpp"
#ifndef __meta__
    #include "SkrTextureCompiler/texture_sampler_asset.generated.h" // IWYU pragma: export
#endif

namespace skd
{
namespace asset
{
sreflect_struct(
    guid = "d2fc798b-af43-4865-b953-abba2b6d524a"
    serde = @json
)
SKR_TEXTURE_COMPILER_API STextureSamplerImporter final : public SImporter {
    skr::String jsonPath;

    void* Import(skr_io_ram_service_t*, SCookContext* context) override;
    void  Destroy(void* resource) override;
};

sreflect_struct(guid = "f06d5542-4c20-48e4-819a-16a6ae13b295")
SKR_TEXTURE_COMPILER_API STextureSamplerCooker final : public SCooker {
    bool     Cook(SCookContext* ctx) override;
    uint32_t Version() override { return 1u; }
};
} // namespace asset
} // namespace skd