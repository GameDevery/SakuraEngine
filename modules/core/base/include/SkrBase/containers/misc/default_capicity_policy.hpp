#pragma once
#include "SkrBase/config.h"
#include "SkrBase/misc/debug.h"
#include <limits>

namespace skr::container
{
template <typename T, typename TSize>
inline TSize default_get_grow(TSize expect_size, TSize current_capacity)
{
    constexpr TSize first_grow    = 4;
    constexpr TSize constant_grow = 16;

    SKR_ASSERT(expect_size > current_capacity && expect_size > 0);

    // init data
    TSize result = first_grow;

    // calc grow
    if (current_capacity || expect_size > first_grow)
    {
        result = expect_size + 3 * expect_size / 8 + constant_grow;
    }

    // handle num over flow
    if (expect_size > result)
        result = std::numeric_limits<TSize>::max();

    return result;
}
template <typename T, typename TSize>
inline TSize default_get_shrink(TSize expect_size, TSize current_capacity)
{
    SKR_ASSERT(expect_size <= current_capacity);

    TSize result;
    if (((3 * expect_size) < (2 * current_capacity)) &&
        ((current_capacity - expect_size) > 64 || !expect_size))
    {
        result = expect_size;
    }
    else
    {
        result = current_capacity;
    }

    return result;
}
}; // namespace skr::container