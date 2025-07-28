#pragma once
#include "material_type_asset.hpp" // IWYU pragma: export
#include "SkrRenderer/resources/material_type_resource.hpp"
#ifndef __meta__
    #include "SkrShaderCompiler/assets/material_asset.generated.h" // IWYU pragma: export
#endif

namespace skd::asset
{

sreflect_struct(
    guid = "b38147b2-a5af-40c6-b2bd-185d16ca83ac" serde = @bin | @json)
skr_material_asset_t
{
    uint32_t material_type_version;

    // refers to a material type
    resource::AsyncResource<renderer::MaterialTypeResource> material_type;

    // properties are mapped to shader parameter bindings (scalars, vectors, matrices, buffers, textures, etc.)
    Vector<skr_material_value_t> override_values;

    // final values for options
    // options can be provided variantly by each material, if not provided, the default value will be used
    Vector<skr_shader_option_instance_t> switch_values;

    // default value for options
    // options can be provided variantly at runtime, if not provided, the default value will be used
    Vector<skr_shader_option_instance_t> option_defaults;
};

sreflect_struct(
    guid = "b5fc88c3-0770-4332-9eda-9e283e29c7dd" serde = @json)
SKR_SHADER_COMPILER_API MaterialImporter final : public Importer
{
    String jsonPath;

    // stable hash for material paramters, can be used by PSO cache or other places.
    uint64_t identity[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

    void* Import(skr::io::IRAMService*, CookContext * context) override;
    void Destroy(void* resource) override;
};

// Cookers

sreflect_struct(guid = "0e3b550f-cdd7-4796-a6d5-0c457e0640bd")
SKR_SHADER_COMPILER_API MaterialCooker final : public Cooker
{
    bool Cook(CookContext * ctx) override;
    uint32_t Version() override { return kDevelopmentVersion; }
};

} // namespace skd::asset