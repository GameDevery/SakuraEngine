#pragma once
#include "type_usings.hpp" // IWYU pragma: export

#define gpu_database(...) struct

namespace skr::gpu
{

template <typename T>
struct TableEntry;

template <typename T>
struct Row
{
    TableEntry<T>* entry;
};
static_assert(sizeof(Row<float>) == 2 * sizeof(uint32_t));

template <typename T, uint32_t N>
struct FixedRange
{
    TableEntry<T>* entry;
};
static_assert(sizeof(FixedRange<float, 1>) == 2 * sizeof(uint32_t));

template <typename T>
struct Range
{
    TableEntry<T>* entry;
    uint32_t pad;
    uint32_t pad1;
};
static_assert(sizeof(Range<float>) == 4 * sizeof(uint32_t));

} // namespace skr::gpu