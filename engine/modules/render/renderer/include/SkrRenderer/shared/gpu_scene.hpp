#pragma once
#include "database.hpp"
#if !defined(__meta__) && !defined(__CPPSL__)
    #include "SkrRenderer/shared/gpu_scene.generated.h" // IWYU pragma: export
#endif

namespace skr::gpu
{

sreflect_managed_component(guid = "74e60480-7324-48d7-a174-bc5e404fe0bc") /*gpu.table=@aos*/
Material
{
    uint32_t global_index;
    uint32_t basecolor_tex;
    uint32_t metallic_roughness_tex;
    uint32_t emission_tex;
};

sreflect_managed_component(guid = "62af1ab3-7762-4413-97f9-fab30d9232c0")/*gpu.table=@aos*/
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

sreflect_managed_component(guid = "73a52be2-4247-4320-b30c-f1fbd5a1766a")/*gpu.table=@aos*/
Instance
{
    float4x4 transform;
    Range<Material> materials;
    Range<Primitive> primitives;
};

template <>
struct GPUDatablock<Material>
{
public:
    inline static constexpr uint32_t Size = 
        GPUDatablock<uint32_t>::Size + GPUDatablock<uint32_t>::Size + 
        GPUDatablock<uint32_t>::Size + GPUDatablock<uint32_t>::Size;
    
    GPUDatablock<Material>(const Material& v)
        : global_index(v.global_index), basecolor_tex(v.basecolor_tex), 
          metallic_roughness_tex(v.metallic_roughness_tex), emission_tex(v.emission_tex)
    {

    }
    operator Material() const 
    { 
        Material output;
        output.global_index = global_index;
        output.basecolor_tex = basecolor_tex;
        output.metallic_roughness_tex = metallic_roughness_tex;
        output.emission_tex = emission_tex;
        return output;
    }
private:
    GPUDatablock<uint32_t> global_index;
    GPUDatablock<uint32_t> basecolor_tex;
    GPUDatablock<uint32_t> metallic_roughness_tex;
    GPUDatablock<uint32_t> emission_tex;
};

template <>
struct GPUDatablock<Primitive>
{
public:
    inline static constexpr uint32_t Size =
        GPUDatablock<uint32_t>::Size + GPUDatablock<uint32_t>::Size + 
        GPUDatablock<Range<uint3>>::Size + GPUDatablock<Range<float3>>::Size + 
        GPUDatablock<Range<float3>>::Size + GPUDatablock<Range<float3>>::Size + 
        GPUDatablock<Range<float2>>::Size;
        
    GPUDatablock<Primitive>(const Primitive& v)
        : global_index(v.global_index), material_index(v.material_index), 
        triangles(v.triangles), positions(v.positions), normals(v.normals),
        tangents(v.tangents), uvs(v.uvs)
    {

    }
    operator Primitive() const 
    { 
        Primitive output;
        output.global_index = global_index;
        output.material_index = material_index;
        output.triangles = triangles;
        output.positions = positions;
        output.normals = normals;
        output.tangents = tangents;
        output.uvs = uvs;
        return output;
    }
private:
    GPUDatablock<uint32_t> global_index;
    GPUDatablock<uint32_t> material_index;
    GPUDatablock<Range<uint3>> triangles;
    GPUDatablock<Range<float3>> positions;
    GPUDatablock<Range<float3>> normals;
    GPUDatablock<Range<float3>> tangents;
    GPUDatablock<Range<float2>> uvs;
};

template <>
struct GPUDatablock<Instance>
{
public:
    inline static constexpr uint32_t Size = 
        sizeof(GPUDatablock<float4x4>) +
        sizeof(GPUDatablock<Range<Material>>) + sizeof(GPUDatablock<Range<Primitive>>);

    GPUDatablock<Instance>(const Instance& v)
        : transform(v.transform), primitives(v.primitives), materials(v.materials)
    {

    }
    operator Instance() const 
    { 
        Instance output;
        output.transform = transform;
        output.materials = materials;
        output.primitives = primitives;
        return output;
    }
private:
    GPUDatablock<float4x4> transform;
    GPUDatablock<Range<Material>> materials;
    GPUDatablock<Range<Primitive>> primitives;
};

} // namespace skr