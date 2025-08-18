#pragma once
#include "SkrBase/containers/vector/vector_def.hpp"

namespace skr::container
{
template <typename T, typename TSize, bool kConst>
using RingBufferDataRef = VectorDataRef<T, TSize, kConst>;
}