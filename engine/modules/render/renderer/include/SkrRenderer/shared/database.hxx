#pragma once
#include <std/std.hpp>
#include "SkrRenderer/shared/database.hpp"

namespace skr::gpu
{
// GPU
template<typename T, uint32 cache_flags>
struct BufferBytesReader;

template<uint32 cache_flags>
struct [[builtin("buffer")]] BufferBytesReader<void, cache_flags> {
	template<typename T = uint>
	[[callop("BYTE_BUFFER_READ")]] T Load(uint32 byte_index) const;
	
	template<typename T = uint>
	[[callop("BYTE_BUFFER_WRITE")]] void Store(uint32 byte_index, const T& val) requires((cache_flags & BufferFlags::ReadWrite) != 0);

	[[callop("BYTE_BUFFER_LOAD")]] uint Load(uint byte_index) const;
	[[callop("BYTE_BUFFER_LOAD2")]] uint2 Load2(uint byte_index) const;
	[[callop("BYTE_BUFFER_LOAD3")]] uint3 Load3(uint byte_index) const;
	[[callop("BYTE_BUFFER_LOAD4")]] uint4 Load4(uint byte_index) const;

	[[callop("BYTE_BUFFER_STORE")]] void Store(uint byte_index, uint value) requires((cache_flags & BufferFlags::ReadWrite) != 0);
	[[callop("BYTE_BUFFER_STORE2")]] void Store2(uint byte_index, uint2 value) requires((cache_flags & BufferFlags::ReadWrite) != 0);
	[[callop("BYTE_BUFFER_STORE3")]] void Store3(uint byte_index, uint3 value) requires((cache_flags & BufferFlags::ReadWrite) != 0);
	[[callop("BYTE_BUFFER_STORE4")]] void Store4(uint byte_index, uint4 value) requires((cache_flags & BufferFlags::ReadWrite) != 0);
};

template <typename T>
struct GPUDatablock<Row<T>>
{
    uint32_t bindless_index;
    uint32_t index;
    operator Row<T>() const 
    { 
        Row<T> row; 
        row.bindless_index = bindless_index;
        row.instance_index = index;
        return row; 
    }

    template <typename Reader>
    static GPUDatablock<Row<T>> Load(Reader reader, uint32_t offset)
    {
        return reader.template Load<GPUDatablock<Row<T>>>(offset);
    }
};

template <typename T, uint32_t N>
struct GPUDatablock<FixedRange<T, N>>
{
    uint32_t bindless_index;
    uint32_t index;
    operator FixedRange<T, N>() const 
    { 
        FixedRange<T, N> range; 
        range.bindless_index = bindless_index;
        range.index = index;
        return range; 
    }

    template <typename Reader>
    static GPUDatablock<FixedRange<T, N>> Load(Reader reader, uint32_t offset)
    {
        return reader.template Load<GPUDatablock<FixedRange<T, N>>>(offset);
    }
};

template <typename T>
struct GPUDatablock<Range<T>>
{
    uint32_t bindless_index;
    uint32_t index;
    uint32_t count;
    operator Range<T>() const 
    { 
        Range<T> range; 
        range.bindless_index = bindless_index;
        range.index = index;
        range.count = count;
        return range; 
    }

    template <typename Reader>
    static GPUDatablock<Range<T>> Load(Reader reader, uint32_t offset)
    {
        return reader.template Load<GPUDatablock<Range<T>>>(offset);
    }
};

template <>
struct GPUDatablock<uint32_t>
{
    uint32_t f0;
    operator uint32_t() const { return f0; }

    template <typename Reader>
    static GPUDatablock<uint32_t> Load(Reader reader, uint32_t offset)
    {
        return reader.template Load<GPUDatablock<uint32_t>>(offset);
    }
};

template <>
struct GPUDatablock<float>
{
    float f0;
    operator float() const { return f0; }

    template <typename Reader>
    static GPUDatablock<float> Load(Reader reader, uint32_t offset)
    {
        return reader.template Load<GPUDatablock<float>>(offset);
    }
};

template <>
struct GPUDatablock<float3>
{
    uint3 raw;
    operator float3() const { return asfloat(raw); }
    
    template <typename Reader>
    static GPUDatablock<float3> Load(Reader buffer, uint32_t offset)
    {
        GPUDatablock<float3> raw;
        raw.raw = buffer.Load3(offset);
        return raw;
    }
};

template <>
struct GPUDatablock<uint3>
{
    uint3 raw;
    operator uint3() const { return raw; }
    
    template <typename Reader>
    static GPUDatablock<uint3> Load(Reader buffer, uint32_t offset)
    {
        GPUDatablock<uint3> raw;
        raw.raw = buffer.Load3(offset);
        return raw;
    }
};

template <>
struct GPUDatablock<Primitive>
{
    GPUDatablock<Row<uint3>> triangles;
    GPUDatablock<Row<float3>> positions;
    GPUDatablock<uint32_t> mat_index;

    template <typename Reader>
    static GPUDatablock<Primitive> Load(Reader buffer, uint32_t offset)
    {
        return buffer.template Load<GPUDatablock<Primitive>>(offset);
    }
    // GEN
    operator Primitive() const 
    { 
        Primitive output;
        output.triangles = triangles;
        output.positions = positions;
        output.mat_index = mat_index;
        return output;
    }
};

template <typename E>
struct SOAAccessor
{
    SOAAccessor(uint32_t buffer_start, uint32_t suboffset)
        : buffer_start(buffer_start), suboffset(suboffset)
    {

    }
    template <typename T, uint64_t PageSize, uint64_t >
    E Get(ByteAddressBuffer buffer, uint32_t index)
    {

        GPUDatablock<T>::Load(buffer, buffer_start + index * sizeof(GPUDatablock<T>));
    }
    uint32_t buffer_start;
    uint32_t suboffset;
};

void maintest()
{
    ByteAddressBuffer buffer;
    struct HIT { uint32_t inst; uint32 geom; } hit;

    Instance inst = Row<Instance>(hit.inst).Load(buffer);
    Primitive prim = inst.primitives.LoadAt(buffer, hit.geom);
    auto mat_index = prim.mat_index;

    
}

struct PrimitiveTable_SOA
{
    SOAAccessor<Row<uint3>> indices;
    SOAAccessor<Row<float3>> positions;
    SOAAccessor<uint32_t> mat_index;
    ByteAddressBuffer buffer;
};
} // namespace skr::gpu