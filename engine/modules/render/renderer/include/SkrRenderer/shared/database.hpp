#pragma once
#include "soa_layout.hpp"
#ifdef __CPPSL__
    #include "detail/database.hxx" // IWYU pragma: export
#else
    #include "detail/database.hpp" // IWYU pragma: export
#endif

namespace skr::gpu
{

gpu_struct(guid = "62af1ab3-7762-4413-97f9-fab30d9232c0")
Primitive
{
    Row<uint3> triangles;
    Row<float3> positions;
    uint32_t mat_index;
};

gpu_struct(guid = "74e60480-7324-48d7-a174-bc5e404fe0bc") 
Material
{
    uint32_t basecolor_tex;
    uint32_t metallic_roughness_tex;
    uint32_t emission_tex;
};

gpu_struct(guid = "73a52be2-4247-4320-b30c-f1fbd5a1766a")
Instance
{
    Range<Primitive> primitives;
    Range<Material> materials;
};

template<typename T>
inline constexpr bool is_table_range_v = false;
template<typename U>
inline constexpr bool is_table_range_v<Row<U>> = true;
template<typename U>
inline constexpr bool is_table_range_v<Range<U>> = true;
template<typename U, uint32_t N>
inline constexpr bool is_table_range_v<FixedRange<U, N>> = true;

template <typename T>
concept GpuTableElement = 
    std::is_same_v<T, float> || std::is_same_v<T, float2> || std::is_same_v<T, float3> || std::is_same_v<T, float4> ||
    std::is_same_v<T, uint32_t> || std::is_same_v<T, uint2> || std::is_same_v<T, uint3> || std::is_same_v<T, uint4> ||
    std::is_same_v<T, float3x3> || std::is_same_v<T, float4x4> ||
    is_table_range_v<T>;

} // skr::gpu