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
        : _instance_index(instance), _bindless_index(bindless_loc)
    {

    }

    T Load(ByteAddressBuffer buffers[], uint32_t byte_offset = 0) const 
    {
        static constexpr bool is_aos = true;
        if constexpr (is_aos)
            return buffers[_bindless_index].Load<GPUDatablock<T>>(byte_offset + _instance_index * GPUDatablock<T>::Size);
    }

    T Load(ByteAddressBuffer buffer, uint32_t byte_offset = 0) const 
    {
        static constexpr bool is_aos = true;
        if constexpr (is_aos)
            return buffer.Load<GPUDatablock<T>>(byte_offset + _instance_index * GPUDatablock<T>::Size);
    }

    void Store(RWByteAddressBuffer buffer, const T& v, uint32_t byte_offset = 0)
    {
        static constexpr bool is_aos = true;
        if constexpr (is_aos)
            buffer.Store(byte_offset + _instance_index * GPUDatablock<T>::Size, GPUDatablock<T>(v));
    }

    auto bindless_index() const { return _bindless_index; }

private:
    friend struct GPUDatablock<Row<T>>;
    uint32_t _instance_index = 0;
    uint32_t _bindless_index = ~0;
};

template <typename T, uint32_t N>
struct FixedRange
{
public:
    FixedRange() = default;
    FixedRange(uint32_t first_instance, uint32_t bindless_loc = ~0)
        : _first_instance(first_instance), _bindless_index(bindless_loc)
    {

    }

    template <typename ByteBufferType>
    T LoadAt(ByteAddressBuffer buffer, uint32_t instance_index, uint32_t byte_offset = 0) const
    {
        static constexpr bool is_aos = true;
        if constexpr (is_aos)
            return buffer.Load<GPUDatablock<T>>(byte_offset + (_first_instance + instance_index) * GPUDatablock<T>::Size);
    }

    void StoreAt(RWByteAddressBuffer buffer, uint32_t instance_index, const T& v, uint32_t byte_offset = 0)
    {
        static constexpr bool is_aos = true;
        if constexpr (is_aos)
            buffer.Store(byte_offset + (_first_instance + instance_index) * GPUDatablock<T>::Size, GPUDatablock<T>(v));
    }

private:
    friend struct GPUDatablock<FixedRange<T, N>>;
    uint32_t _first_instance = 0;
    uint32_t _bindless_index = ~0;
};

template <typename T>
struct Range
{
public:
    Range() = default;
    Range(uint32_t first_instance, uint32_t count = ~0, uint32_t bindless_loc = ~0)
        : _first_instance(first_instance), _count(count), _bindless_index(bindless_loc)
    {

    }

    T LoadAt(ByteAddressBuffer buffer, uint32_t instance_index, uint32_t byte_offset = 0) const
    {
        static constexpr bool is_aos = true;
        if constexpr (is_aos)
            return buffer.Load<GPUDatablock<T>>(byte_offset + (_first_instance + instance_index) * GPUDatablock<T>::Size);
    }

    T LoadAt(ByteAddressBuffer buffers[], uint32_t instance_index, uint32_t byte_offset = 0) const
    {
        static constexpr bool is_aos = true;
        if constexpr (is_aos)
            return buffers[_bindless_index].Load<GPUDatablock<T>>(byte_offset + ((_first_instance + instance_index) * GPUDatablock<T>::Size));
    }

    void StoreAt(RWByteAddressBuffer buffer, uint32_t instance_index, const T& v, uint32_t byte_offset = 0)
    {
        static constexpr bool is_aos = true;
        if constexpr (is_aos)
            buffer.Store(byte_offset + (_first_instance + instance_index) * GPUDatablock<T>::Size, GPUDatablock<T>(v));
    }

    uint32_t count() const { return _count; }

private:
    friend struct GPUDatablock<Range<T>>;
    uint32_t _first_instance = 0;
    uint32_t _count = 0;
    uint32_t _bindless_index = ~0;
};

} // namespace skr::gpu