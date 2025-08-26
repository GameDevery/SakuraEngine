#pragma once
#include "./../attributes.hpp"
#include "./../types/vec.hpp"

template <concepts::struct_type Type>
struct [[builtin("constant_buffer")]] ConstantBuffer : public Type
{
    
};

template<typename Type, uint32 cache_flags = BufferFlags::ReadOnly>
struct [[builtin("buffer")]] Buffer {
	using ElementType = Type;
	inline static constexpr auto flags = cache_flags;

	[[callop("BUFFER_READ")]] const Type& Load(uint32 loc) const;
	[[callop("BUFFER_WRITE")]] void Store(uint32 loc, const Type& value) requires((cache_flags & BufferFlags::ReadWrite) != 0);
	[[access]] const Type& operator[](uint32 loc) const;
	[[access]] Type& operator[](uint32 loc) requires((cache_flags & BufferFlags::ReadWrite) != 0);
};
template<>
struct [[builtin("buffer")]] Buffer<int32, BufferFlags::ReadWrite> {
	using ElementType = int32;

	[[callop("BUFFER_READ")]] int32 Load(uint32 loc) const;
	[[callop("BUFFER_WRITE")]] void Store(uint32 loc, int32 value);
	[[access]] const int32& operator[](uint32 loc) const;
	[[access]] int32& operator[](uint32 loc);
};
template<>
struct [[builtin("buffer")]] Buffer<uint32, BufferFlags::ReadWrite> {
	using ElementType = uint32;

	[[callop("BUFFER_READ")]] uint32 Load(uint32 loc) const;
	[[callop("BUFFER_WRITE")]] void Store(uint32 loc, uint32 value);
	[[access]] const uint32& operator[](uint32 loc) const;
	[[access]] uint32& operator[](uint32 loc);
};
template<>
struct [[builtin("buffer")]] Buffer<float, BufferFlags::ReadWrite> {
	using ElementType = float;

	[[callop("BUFFER_READ")]] float Load(uint32 loc) const;
	[[callop("BUFFER_WRITE")]] void Store(uint32 loc, float value);
	[[access]] const float& operator[](uint32 loc) const;
	[[access]] float& operator[](uint32 loc);
};
template<uint32 cache_flags>
struct [[builtin("buffer")]] Buffer<void, cache_flags> {
	
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

template<typename Type>
using RWBuffer = Buffer<Type, BufferFlags::ReadWrite>;

template<typename Type>
using StructuredBuffer = Buffer<Type, BufferFlags::ReadOnly>;
template<typename Type>
using RWStructuredBuffer = Buffer<Type, BufferFlags::ReadWrite>;

using ByteAddressBuffer = Buffer<void, BufferFlags::ReadOnly>;
using RWByteAddressBuffer = Buffer<void, BufferFlags::ReadWrite>;