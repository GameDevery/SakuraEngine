#pragma once
#include "soa_layout.hpp"
#if !defined(__meta__) && !defined(__CPPSL__)
    #include "SkrRenderer/shared/gpu_scene.generated.h" // IWYU pragma: export
#endif

namespace skr
{

sreflect_managed_component(guid = "7d7ae068-1c62-46a0-a3bc-e6b141c8e56d")
GPUSceneObjectToWorld
{
    gpu::float4x4 matrix;
};

sreflect_managed_component(guid = "d108318f-a1c2-4f64-b82f-63e7914773c8")
GPUSceneInstanceColor
{
    gpu::float4 color;
};

sreflect_managed_component(guid = "e72c5ea1-9e31-4649-b190-45733b176760")
GPUSceneInstanceEmission
{
    gpu::float4 color;
};

struct BufferEntry
{
    skr::gpu::uint32 buffer = ~0;
    skr::gpu::uint32 offset = ~0;
};

sreflect_managed_component(guid = "03035b31-a526-4fa7-b062-56da0849d5b9")
PBRMaterial
{
    struct Group
    {
        uint32_t basecolor_tex;
        uint32_t metallic_roughness_tex;
        uint32_t emission_tex;
    } entries[256];
};

sreflect_managed_component(guid = "a434196c-7a99-4dee-8715-e422124288e2")
GPUSceneGeometryBuffers
{
    struct Group
    {
        BufferEntry index;
        BufferEntry pos;
        BufferEntry uv;
        BufferEntry normal;
        BufferEntry tangent;
        uint32_t material_index;
    } entries[256];
};

using DefaultGPUSceneLayout = PagedLayout<16384, // 16K instances per page
    GPUSceneObjectToWorld,
    GPUSceneInstanceColor,
    GPUSceneInstanceEmission,
    GPUSceneGeometryBuffers
>;

} // namespace skr