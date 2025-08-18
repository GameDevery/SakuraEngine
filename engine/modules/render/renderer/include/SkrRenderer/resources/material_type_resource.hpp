#pragma once
#include "SkrBase/config.h"
#include "SkrRenderer/resources/shader_resource.hpp"
#include "SkrRenderer/resources/shader_meta_resource.hpp"

#ifndef __meta__
    #include "SkrRenderer/resources/material_type_resource.generated.h" // IWYU pragma: export
#endif

namespace skr::renderer
{
using MaterialPropertyName = skr::String;

sreflect_enum_class(
    guid = "4003703a-dde4-4f11-93a6-6c460bac6357";
    serde = @bin | @json;)
EMaterialPropertyType : uint32_t{
    BOOL,
    FLOAT,
    FLOAT2,
    FLOAT3,
    FLOAT4,
    DOUBLE,
    TEXTURE,
    BUFFER,
    SAMPLER,
    STATIC_SAMPLER,
    COUNT
};

sreflect_enum(
    guid = "575331c4-785f-4a4d-b320-4490bb7a6180";
    serde = @bin | @json;)
EMaterialBlendMode : uint32_t{
    Opaque,
    Blend,
    Mask,
    Count
};

sreflect_struct(
    guid = "6cdbf15e-67c1-45c1-a4e9-417c81299dae";
    serde = @bin | @json;)
MaterialProperty
{
    EMaterialPropertyType prop_type;
    MaterialPropertyName name;
    String display_name;
    String description;

    double default_value = 0.0;
    double min_value = 0.0;
    double max_value = DBL_MAX;

    float4 default_vec;
    float4 min_vec = { 0.0f, 0.0f, 0.0f, 0.0f };
    float4 max_vec = { FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX };

    SResourceHandle default_resource;
};

// material value setter mainly used for devlopment-time material editing
// at runtime we use skr_material_value_$(type)_t
sreflect_struct(
    guid = "46de11b4-6beb-4ab9-b9f8-f5c07ceeb8a5";
    serde = @bin | @json;)
MaterialValue
{
    EMaterialPropertyType prop_type;
    MaterialPropertyName slot_name;

    double value = 0.0;
    skr_float4_t vec = { 0.0f, 0.0f, 0.0f, 0.0f };
    SResourceHandle resource;
};

sreflect_struct(
    guid = "ed2e3476-90a3-4f2f-ac97-808f63d1eb11";
    serde = @bin | @json;)
MaterialPass
{
    String pass;
    Vector<skr_shader_collection_handle_t> shader_resources;
    Vector<EMaterialBlendMode> blend_modes;
    bool two_sided = false;
};

sreflect_struct(
    guid = "83264b35-3fde-4fff-8ee1-89abce2e445b";
    serde = @bin | @json;)
MaterialTypeResource
{
    uint32_t version;

    Vector<MaterialPass> passes;
    Vector<MaterialValue> default_values;
    Vector<skr_shader_option_instance_t> switch_defaults;
    Vector<skr_shader_option_instance_t> option_defaults;
    skr_vertex_layout_id vertex_type;
};

struct SKR_RENDERER_API MaterialTypeFactory : public resource::ResourceFactory
{
    struct Root
    {
        SRenderDeviceId render_device = nullptr;
    };

    virtual ~MaterialTypeFactory() = default;
    float AsyncSerdeLoadFactor() override { return 1.f; }
    [[nodiscard]] static MaterialTypeFactory* Create(const Root& root);
    static void Destroy(MaterialTypeFactory* factory);
};
} // namespace skr::renderer