#pragma once
#include "type_usings.hxx" // IWYU pragma: export

#define gpu_struct(...) struct
#define gpu_table(...) struct

namespace skr::gpu
{

template <typename T> 
struct GPUDatablock;

template <typename T>
struct Row
{
public:
    Row() = default;
    Row(uint32_t instance, uint32_t bindless_loc = ~0)
        : _bindless_index(bindless_loc), _instance_index(instance)
    {

    }

    T Load(ByteAddressBuffer buffer, uint32_t byte_offset = 0)
    {
        static constexpr bool is_aos = true;
        if constexpr (is_aos)
            return buffer.Load(byte_offset + _instance_index * GPUDatablock<T>::Size);
    }

    void StoreAt(RWByteAddressBuffer buffer, const T& v, uint32_t byte_offset = 0)
    {
        static constexpr bool is_aos = true;
        if constexpr (is_aos)
            buffer.Store(byte_offset + _instance_index * GPUDatablock<T>::Size, v);
    }

private:
    friend struct GPUDatablock<Row<T>>;
    uint32_t _bindless_index = ~0;
    uint32_t _instance_index;
};
static_assert(sizeof(Row<float>) == 2 * sizeof(uint32_t));


template <typename T, uint32_t N>
struct FixedRange
{
public:
    FixedRange() = default;
    FixedRange(uint32_t first_instance, uint32_t bindless_loc = ~0)
        : _bindless_index(bindless_loc), _first_instance(first_instance)
    {

    }

    template <typename ByteBufferType>
    T LoadAt(ByteAddressBuffer buffer, uint32_t instance_index, uint32_t byte_offset = 0)
    {
        static constexpr bool is_aos = true;
        if constexpr (is_aos)
            return buffer.Load(byte_offset + (_first_instance + instance_index) * GPUDatablock<T>::Size);
    }

    void StoreAt(RWByteAddressBuffer buffer, uint32_t instance_index, const T& v, uint32_t byte_offset = 0)
    {
        static constexpr bool is_aos = true;
        if constexpr (is_aos)
            buffer.Store(byte_offset + (_first_instance + instance_index) * GPUDatablock<T>::Size, v);
    }

private:
    friend struct GPUDatablock<FixedRange<T, N>>;
    uint32_t _bindless_index;
    uint32_t _first_instance;
};
static_assert(sizeof(FixedRange<float, 1>) == 2 * sizeof(uint32_t));

template <typename T>
struct Range
{
public:
    Range() = default;
    Range(uint32_t first_instance, uint32_t count = ~0, uint32_t bindless_loc = ~0)
        : _bindless_index(bindless_loc), _first_instance(first_instance), _count(count)
    {

    }

    T LoadAt(ByteAddressBuffer buffer, uint32_t instance_index, uint32_t byte_offset = 0)
    {
        static constexpr bool is_aos = true;
        if constexpr (is_aos)
            return buffer.Load(byte_offset + (_first_instance + instance_index) * GPUDatablock<T>::Size);
    }

    void StoreAt(RWByteAddressBuffer buffer, uint32_t instance_index, const T& v, uint32_t byte_offset = 0)
    {
        static constexpr bool is_aos = true;
        if constexpr (is_aos)
            buffer.Store(byte_offset + (_first_instance + instance_index) * GPUDatablock<T>::Size, v);
    }

    uint32_t count() const { return _count; }

private:
    friend struct GPUDatablock<Range<T>>;
    uint32_t _bindless_index;
    uint32_t _first_instance;
    uint32_t _count;
    uint32_t pad;
};
static_assert(sizeof(Range<float>) == 4 * sizeof(uint32_t));

} // namespace skr::gpu