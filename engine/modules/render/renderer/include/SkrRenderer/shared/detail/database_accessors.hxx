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
    Row(uint32_t instance, uint32_t buffer_offset = 0, uint32_t bindless_loc = ~0)
        : _instance_index(instance), _buffer_offset(buffer_offset), _bindless_index(bindless_loc)
    {

    }

    T Load(ByteAddressBuffer buffers[], uint32_t byte_offset = 0) const 
    {
        return buffers[_bindless_index].Load<GPUDatablock<T>>(_buffer_offset + byte_offset + _instance_index * GPUDatablock<T>::Size);
    }

    T Load(ByteAddressBuffer buffer, uint32_t byte_offset = 0) const 
    {
        return buffer.Load<GPUDatablock<T>>(_buffer_offset + byte_offset + _instance_index * GPUDatablock<T>::Size);
    }

    void Store(RWByteAddressBuffer buffer, const T& v, uint32_t byte_offset = 0)
    {
        buffer.Store(_buffer_offset + byte_offset + _instance_index * GPUDatablock<T>::Size, GPUDatablock<T>(v));
    }

    auto bindless_index() const { return _bindless_index; }

private:
    friend struct GPUDatablock<Row<T>>;
    uint32_t _instance_index = 0;
    uint32_t _buffer_offset = 0;
    uint32_t _bindless_index = ~0;
};

template <typename T, uint32_t N>
struct FixedRange
{
public:
    FixedRange() = default;
    FixedRange(uint32_t first_instance, uint32_t buffer_offset = 0, uint32_t bindless_loc = ~0)
        : _first_instance(first_instance), _buffer_offset(buffer_offset), _bindless_index(bindless_loc)
    {

    }

    template <typename ByteBufferType>
    T Load(ByteAddressBuffer buffers[], uint32_t instance_index, uint32_t byte_offset = 0) const
    {
        return buffers[_bindless_index].Load<GPUDatablock<T>>(_buffer_offset + byte_offset + (_first_instance + instance_index) * GPUDatablock<T>::Size);
    }

    template <typename ByteBufferType>
    T Load(ByteAddressBuffer buffer, uint32_t instance_index, uint32_t byte_offset = 0) const
    {
        return buffer.Load<GPUDatablock<T>>(_buffer_offset + byte_offset + (_first_instance + instance_index) * GPUDatablock<T>::Size);
    }

    void Store(RWByteAddressBuffer buffer, uint32_t instance_index, const T& v, uint32_t byte_offset = 0)
    {
        buffer.Store(_buffer_offset + byte_offset + (_first_instance + instance_index) * GPUDatablock<T>::Size, GPUDatablock<T>(v));
    }

private:
    friend struct GPUDatablock<FixedRange<T, N>>;
    uint32_t _first_instance = 0;
    uint32_t _buffer_offset = 0;
    uint32_t _bindless_index = ~0;
};

template <typename T>
struct Range
{
public:
    Range() = default;
    Range(uint32_t first_instance, uint32_t count = ~0, uint32_t buffer_offset = 0, uint32_t bindless_loc = ~0)
        : _first_instance(first_instance), _count(count), _buffer_offset(buffer_offset), _bindless_index(bindless_loc)
    {

    }

    T Load(ByteAddressBuffer buffer, uint32_t instance_index, uint32_t byte_offset = 0) const
    {
        return buffer.Load<GPUDatablock<T>>(_buffer_offset + byte_offset + (_first_instance + instance_index) * GPUDatablock<T>::Size);
    }

    T Load(ByteAddressBuffer buffers[], uint32_t instance_index, uint32_t byte_offset = 0) const
    {
        return buffers[_bindless_index].Load<GPUDatablock<T>>(_buffer_offset + byte_offset + ((_first_instance + instance_index) * GPUDatablock<T>::Size));
    }

    void Store(RWByteAddressBuffer buffer, uint32_t instance_index, const T& v, uint32_t byte_offset = 0)
    {
        buffer.Store(_buffer_offset + byte_offset + (_first_instance + instance_index) * GPUDatablock<T>::Size, GPUDatablock<T>(v));
    }

    uint32_t count() const { return _count; }

private:
    friend struct GPUDatablock<Range<T>>;
    uint32_t _first_instance = 0;
    uint32_t _count = 0;
    uint32_t _buffer_offset = 0;
    uint32_t _bindless_index = ~0;
};

template <typename T, uint32_t Idx, uint32_t PageSize>
struct SOAAccessor
{
public:
    void operator=(const T& v)
    {
        GPUDatablock<T> data(v);
    }

private:
    ByteAddressBuffer _buffer;
    uint32_t _buffer_offset = 0;
};

template <typename T, uint32_t Idx, uint32_t PageSize>
struct SOARow
{
public:
    SOAAccessor<T, Idx, PageSize> operator[](uint32_t idx)
    {

    }

private:
    ByteAddressBuffer _buffer;
};


} // namespace skr::gpu