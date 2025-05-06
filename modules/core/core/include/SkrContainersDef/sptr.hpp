#pragma once
#include "detail/sptr.hpp"      // IWYU pragma: export
#include "detail/sweak_ptr.hpp" // IWYU pragma: keep

// TODO. SRC/WRC 与 SPtr/WPtr API 区分
// TODO. WRC 采取一个额外的 Core 来管理 WeakRefCount
namespace skr
{
template <typename T, bool EmbedRC>
struct SPtrHelper;
template <typename T>
using SPtr = SPtrHelper<T, true>;
} // namespace skr