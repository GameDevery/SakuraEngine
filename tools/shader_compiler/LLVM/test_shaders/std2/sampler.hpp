#pragma once
#include "./../std/std.hpp"
#include "attributes.hpp"

namespace skr::shader {

struct [[builtin("sampler")]] Sampler 
{
    template <typename T, uint32 Flags>
    [[callop("SAMPLE2D")]] vec<T, 4> Sample(Image<T, Flags>& image, float2 uv);
};

} // namespace skr::shader