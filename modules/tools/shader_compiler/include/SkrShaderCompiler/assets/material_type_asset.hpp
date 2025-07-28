#pragma once
#include "SkrRenderer/fwd_types.h"
#include "SkrContainersDef/vector.hpp"
#include "SkrContainersDef/string.hpp"
#include "SkrToolCore/asset/importer.hpp"
#include "SkrToolCore/asset/cooker.hpp"
#ifndef __meta__
    #include "SkrShaderCompiler/assets/material_type_asset.generated.h" // IWYU pragma: export
#endif

namespace skd::asset
{
sreflect_struct(
    guid = "329fddb1-73a6-4b4b-8f9f-f4acca58a6e5" serde = @bin | @json)
MaterialTypeAsset
{
    uint32_t version;

    // shader assets
    Vector<renderer::MaterialPass> passes;

    // properties are mapped to shader parameter bindings (scalars, vectors, matrices, buffers, textures, etc.)
    Vector<renderer::MaterialProperty> properties;

    // default value for options
    // options can be provided variantly by each material, if not provided, the default value will be used
    Vector<renderer::ShaderOptionInstance> switch_defaults;

    // default value for options
    // options can be provided variantly at runtime, if not provided, the default value will be used
    Vector<renderer::ShaderOptionInstance> option_defaults;

    skr_vertex_layout_id vertex_type;
};

sreflect_struct(
    guid = "c0fc5581-f644-4752-bb30-0e7f652533b7" serde = @json)
SKR_SHADER_COMPILER_API MaterialTypeImporter final : public Importer
{
    String jsonPath;

    void* Import(skr_io_ram_service_t*, CookContext * context) override;
    void Destroy(void* resource) override;
};

sreflect_struct(guid = "816f9dd4-9a49-47e5-a29a-3bdf7241ad35")
SKR_SHADER_COMPILER_API MaterialTypeCooker final : public Cooker
{
    bool Cook(CookContext * ctx) override;
    uint32_t Version() override { return kDevelopmentVersion; }
};

} // namespace skd::asset