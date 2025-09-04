#pragma once
#include "./../attributes.hxx"
#include "./../types/vec.hxx"

struct [[builtin("sampler")]] Sampler 
{

};

using SamplerState = Sampler;

template <concepts::arithmetic_scalar T, uint32 cache_flag = TextureFlags::ReadOnly>
struct [[builtin("texture2d")]] Tex2D
{
    using ElementType = T;

    [[callop("TEXTURE_SIZE")]] uint2 Size();
    [[callop("TEXTURE_READ")]] vec<T, 4> Load(uint2 coord);
    [[callop("TEXTURE_WRITE")]] void Store(uint2 coord, vec<T, 4> val) requires(cache_flag == TextureFlags::ReadWrite);

    [[callop("TEXTURE_SAMPLE")]] vec<T, 4> Sample(const SamplerState&, float2);
    [[callop("TEXTURE_SAMPLE_LEVEL")]] vec<T, 4> SampleLevel(const SamplerState&, float2, uint);
};

template <concepts::arithmetic_scalar T, uint32 cache_flag = TextureFlags::ReadOnly>
struct [[builtin("texture3d")]] Tex3D
{
    using ElementType = T;
    [[callop("TEXTURE_SIZE")]] uint3 Size();
    [[callop("TEXTURE_READ")]] vec<T, 4> Load(uint3 coord);
    [[callop("TEXTURE_WRITE")]] void Store(uint3 coord, vec<T, 4> val) requires(cache_flag == TextureFlags::ReadWrite);
    
    [[callop("TEXTURE_SAMPLE")]] vec<T, 4> Sample(const SamplerState&, float3);
    [[callop("TEXTURE_SAMPLE_LEVEL")]] vec<T, 4> SampleLevel(const SamplerState&, float3, uint);
};

template <concepts::arithmetic_scalar T, uint32 cache_flag = TextureFlags::ReadOnly>
struct [[builtin("texture2d_array")]] Tex2DArray
{
    using ElementType = T;
    [[callop("TEXTURE_SIZE")]] uint3 Size();
    [[access]] vec<T, 4> operator[](uint3 pos);

    void GetDimensions(uint& x, uint& y, uint& z)
    {
        uint3 _size = Size();
        x = _size.x; y = _size.y; z = _size.z;
    }
};

template <concepts::arithmetic_scalar T, uint32 cache_flag = TextureFlags::ReadOnly>
struct [[builtin("texture3d_array")]] Tex3DArray
{
    static_assert(cache_flag != TextureFlags::ReadWrite);
    using ElementType = T;
    
};

template <concepts::arithmetic_scalar T, uint32 cache_flag = TextureFlags::ReadOnly>
struct [[builtin("texture_cube")]] TexCube
{
    static_assert(cache_flag != TextureFlags::ReadWrite);
    using ElementType = T;

    [[callop("TEXTURE_SAMPLE")]] vec<T, 4> SampleLevel(const SamplerState&, float3);
    [[callop("TEXTURE_SAMPLE_LEVEL")]] vec<T, 4> SampleLevel(const SamplerState&, float3, uint);
};

template <concepts::arithmetic_scalar_or_vec T = float>
using Texture2D = Tex2D<scalar_type<T>, TextureFlags::ReadOnly>;

template <concepts::arithmetic_scalar_or_vec T = float>
using RWTexture2D = Tex2D<scalar_type<T>, TextureFlags::ReadWrite>;

template <concepts::arithmetic_scalar_or_vec T = float>
using Texture2DArray = Tex2DArray<scalar_type<T>, TextureFlags::ReadOnly>;

template <concepts::arithmetic_scalar_or_vec T = float>
using RWTexture2DArray = Tex2DArray<scalar_type<T>, TextureFlags::ReadWrite>;

template <concepts::arithmetic_scalar_or_vec T = float>
using Texture3D = Tex3D<scalar_type<T>, TextureFlags::ReadOnly>;

template <concepts::arithmetic_scalar_or_vec T = float>
using RWTexture3D = Tex3D<scalar_type<T>, TextureFlags::ReadWrite>;

template <concepts::arithmetic_scalar_or_vec T = float>
using Texture3DArray = Tex3DArray<scalar_type<T>, TextureFlags::ReadOnly>;

template <concepts::arithmetic_scalar_or_vec T = float>
using TextureCube = TexCube<scalar_type<T>, TextureFlags::ReadOnly>;

template <concepts::arithmetic_scalar_or_vec T = float>
using TextureCubeArray = Tex3DArray<scalar_type<T>, TextureFlags::ReadOnly>;