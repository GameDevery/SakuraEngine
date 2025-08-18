#pragma once
#include "SkrCore/memory/memory.h"
#include <vector> // IWYU pragma: export

namespace skr
{

template<typename T, typename Alloc = skr_stl_allocator<T>>
using stl_vector = std::vector<T, Alloc>;

template<typename T, typename Alloc = skr_stl_allocator<T>>
using STLVector = std::vector<T, Alloc>;

} // namespace skr