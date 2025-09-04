#pragma once
#include "SkrRenderer/shared/database.hpp"
#if !defined(__meta__) && !defined(__CPPSL__)
    #include "SkrRenderer/shared/gpu_scene.generated.h" // IWYU pragma: export
#endif

namespace skr::gpu
{

sreflect_managed_component(guid = "74e60480-7324-48d7-a174-bc5e404fe0bc" gpu.aos=@enable)
Material
{
    uint32_t global_index;
    uint32_t basecolor_tex;
    uint32_t metallic_roughness_tex;
    uint32_t emission_tex;
};

sreflect_managed_component(guid = "62af1ab3-7762-4413-97f9-fab30d9232c0" gpu.aos=@enable)
Primitive
{
    uint32_t global_index;
    uint32_t material_index;
    Range<uint3> triangles;
    Range<float3> positions;
    Range<float3> normals;
    Range<float3> tangents;
    Range<float2> uvs;
};

sreflect_managed_component(guid = "73a52be2-4247-4320-b30c-f1fbd5a1766a" gpu.aos=@enable)
Instance
{
    float4x4 transform;
    Range<Material> materials;
    Range<Primitive> primitives;
};

} // namespace skr

#if !defined(__meta__)
    #include "SkrRenderer/shared/gpu_scene.datablocks.h" // IWYU pragma: export
#endif