#pragma once
#include "./../attributes.hpp"
#include "./../types/vec.hpp"

namespace skr::shader
{

template <concepts::arithmetic_scalar T, uint32 cache_flag = TextureFlags::ReadOnly>
struct [[builtin("image")]] Tex2D
{
    using ElementType = T;

    [[callop("TEXTURE_SIZE")]] uint2 Size();
    [[callop("TEXTURE_READ")]] vec<T, 4> Load(uint2 coord);
    [[callop("TEXTURE_WRITE")]] void Store(uint2 coord, vec<T, 4> val) requires(cache_flag == TextureFlags::ReadWrite);
};

template <concepts::arithmetic_scalar T, uint32 cache_flag = TextureFlags::ReadOnly>
struct [[builtin("volume")]] Tex3D
{
    using ElementType = T;
    [[callop("TEXTURE_SIZE")]] uint3 Size();
    [[callop("TEXTURE_READ")]] vec<T, 4> Load(uint3 coord);
    [[callop("TEXTURE_WRITE")]] void Store(uint3 coord, vec<T, 4> val) requires(cache_flag == TextureFlags::ReadWrite);
};

template <concepts::arithmetic_scalar T = float>
using Texture2D = Tex2D<T, TextureFlags::ReadOnly>;

template <concepts::arithmetic_scalar T = float>
using RWTexture2D = Tex2D<T, TextureFlags::ReadWrite>;

} // namespace skr::shader