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
    data_layout::float4x4 matrix;
};

sreflect_managed_component(guid = "d108318f-a1c2-4f64-b82f-63e7914773c8")
GPUSceneInstanceColor
{
    data_layout::float4 color;
};

sreflect_managed_component(guid = "e72c5ea1-9e31-4649-b190-45733b176760")
GPUSceneInstanceEmission
{
    data_layout::float4 color;
};

using DefaultGPUSceneLayout = PagedLayout<16384, // 16K instances per page
    GPUSceneObjectToWorld,
    GPUSceneInstanceColor,
    GPUSceneInstanceEmission
>;

} // namespace skr