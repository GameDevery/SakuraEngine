#pragma once
#include "./../attributes.hpp"
#include "./../types/vec.hpp"

namespace skr::shader
{
trait Filter
{
    static constexpr uint32 POINT = 0;
    static constexpr uint32 LINEAR_POINT = 1;
    static constexpr uint32 LINEAR_LINEAR = 2;
    static constexpr uint32 ANISOTROPIC = 3;
};

trait Address
{
    static constexpr uint32 EDGE = 0;
    static constexpr uint32 REPEAT = 1;
    static constexpr uint32 MIRROR = 2;
    static constexpr uint32 ZERO = 3;
};

template <concepts::arithmetic_scalar T, uint32 cache_flag = TextureFlags::ReadOnly>
struct [[builtin("image")]] Tex2D
{
    using ElementType = T;
	Tex2D() = default;

    [[callop("TEXTURE_READ")]] vec<T, 4> load(uint2 coord);
    [[callop("TEXTURE_SIZE")]] uint2 size();

    [[callop("TEXTURE_WRITE")]] void store(uint2 coord, vec<T, 4> val) requires(cache_flag == TextureFlags::ReadWrite);
};

template <concepts::arithmetic_scalar T, uint32 cache_flag = TextureFlags::ReadOnly>
struct [[builtin("volume")]] Tex3D
{
    using ElementType = T;
	Tex3D() = default;

    [[callop("TEXTURE_READ")]] vec<T, 4> load(uint3 coord);
    [[callop("TEXTURE_SIZE")]] uint3 size();

    [[callop("TEXTURE_WRITE")]] void store(uint3 coord, vec<T, 4> val) requires(cache_flag == TextureFlags::ReadWrite);
};

template <>
struct [[builtin("image")]] Tex2D<float, TextureFlags::ReadOnly>
{
    using ElementType = float;
	Tex2D() = default;

    [[callop("TEXTURE_READ")]] float4 load(uint2 coord);
    [[callop("TEXTURE_SIZE")]] uint2 size();
    [[callop("TEXTURE2D_SAMPLE")]] float4 sample(float2 uv, uint filter, uint address);
    [[callop("TEXTURE2D_SAMPLE_LEVEL")]] float4 sample_level(float2 uv, uint level, uint filter, uint address);
    [[callop("TEXTURE2D_SAMPLE_GRAD")]] float4 sample_grad(float2 uv, float2 ddx, float2 ddy, uint filter, uint address);
    [[callop("TEXTURE2D_SAMPLE_GRAD_LEVEL")]] float4 sample_grad_level(float2 uv, float2 ddx, float2 ddy, float min_mip_level, uint filter, uint address);
};

template <>
struct [[builtin("volume")]] Tex3D<float, TextureFlags::ReadOnly>
{
    using ElementType = float;
	Tex3D() = default;

    [[callop("TEXTURE_READ")]] float4 load(uint3 coord);
    [[callop("TEXTURE_SIZE")]] uint3 size();
    [[callop("TEXTURE3D_SAMPLE")]] float4 sample(float3 uv, uint filter, uint address);
    [[callop("TEXTURE3D_SAMPLE_LEVEL")]] float4 sample_level(float3 uv, uint level, uint filter, uint address);
    [[callop("TEXTURE3D_SAMPLE_GRAD")]] float4 sample_grad(float3 uv, float3 ddx, float3 ddy, uint filter, uint address);
    [[callop("TEXTURE3D_SAMPLE_GRAD_LEVEL")]] float4 sample_grad_level(float3 uv, float3 ddx, float3 ddy, float min_mip_level, uint filter, uint address);
};

template <concepts::arithmetic_scalar T>
using Texture2D = Tex2D<T, TextureFlags::ReadOnly>;

template <concepts::arithmetic_scalar T>
using RWTexture2D = Tex2D<T, TextureFlags::ReadWrite>;

} // namespace skr::shader