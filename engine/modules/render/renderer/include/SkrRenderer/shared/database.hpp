#pragma once
#include "soa_layout.hpp"
#include "detail/database.h"

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
    float4x4 transform;
    Range<Primitive> primitives;
    Range<Material> materials;
};

template <>
struct GPUDatablock<Primitive>
{
public:
    inline static constexpr uint32_t Size = GPUDatablock<Row<uint3>>::Size + GPUDatablock<Row<float3>>::Size + GPUDatablock<uint32_t>::Size;
    GPUDatablock<Primitive>(const Primitive& v)
        : triangles(v.triangles), positions(v.positions), mat_index(v.mat_index)
    {

    }
    operator Primitive() const 
    { 
        Primitive output;
        output.triangles = triangles;
        output.positions = positions;
        output.mat_index = mat_index;
        return output;
    }

private:
    GPUDatablock<Row<uint3>> triangles;
    GPUDatablock<Row<float3>> positions;
    GPUDatablock<uint32_t> mat_index;
};

template <>
struct GPUDatablock<Material>
{
public:
    inline static constexpr uint32_t Size = GPUDatablock<uint32_t>::Size + GPUDatablock<uint32_t>::Size + GPUDatablock<uint32_t>::Size;
    GPUDatablock<Material>(const Material& v)
        : basecolor_tex(v.basecolor_tex), metallic_roughness_tex(v.metallic_roughness_tex), emission_tex(v.emission_tex)
    {

    }
    operator Material() const 
    { 
        Material output;
        output.basecolor_tex = basecolor_tex;
        output.metallic_roughness_tex = metallic_roughness_tex;
        output.emission_tex = emission_tex;
        return output;
    }

private:
    GPUDatablock<uint32_t> basecolor_tex;
    GPUDatablock<uint32_t> metallic_roughness_tex;
    GPUDatablock<uint32_t> emission_tex;
};

template <>
struct GPUDatablock<Instance>
{
public:
    inline static constexpr uint32_t Size = GPUDatablock<Range<Primitive>>::Size + GPUDatablock<Range<Material>>::Size;
    GPUDatablock<Instance>(const Instance& v)
        : primitives(v.primitives), materials(v.materials)
    {

    }
    operator Instance() const 
    { 
        Instance output;
        output.primitives = primitives;
        output.materials = materials;
        return output;
    }

private:
    GPUDatablock<Range<Primitive>> primitives;
    GPUDatablock<Range<Material>> materials;
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