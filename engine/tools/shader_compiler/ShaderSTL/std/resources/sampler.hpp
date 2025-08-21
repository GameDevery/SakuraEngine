#pragma once
#include "./texture.hpp"

namespace skr::shader {

struct [[builtin("sampler")]] Sampler 
{
    template <typename T, uint32 Flags>
    [[callop("SAMPLE2D")]] vec<T, 4> Sample(Tex2D<T, Flags>& image, float2 uv);
};

} // namespace skr::shader