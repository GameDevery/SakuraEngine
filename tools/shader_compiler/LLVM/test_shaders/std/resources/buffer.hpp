#pragma once
#include "./../attributes.hpp"
#include "./../type_traits.hpp"
#include "./../types/vec.hpp"

namespace skr::shader {

template<typename Type, uint32 cache_flags = BufferFlags::ReadOnly>
struct [[builtin("buffer")]] Buffer {
	using ElementType = Type;
	inline static constexpr auto flags = cache_flags;

	[[callop("BUFFER_READ")]] const Type& load(uint32 loc);
	[[callop("BUFFER_WRITE")]] void store(uint32 loc, const Type& value) requires((cache_flags & BufferFlags::ReadWrite) != 0);
};
template<>
struct [[builtin("buffer")]] Buffer<int32, BufferFlags::ReadWrite> {
	using ElementType = int32;

	[[callop("BUFFER_READ")]] int32 load(uint32 loc);
	[[callop("BUFFER_WRITE")]] void store(uint32 loc, int32 value);

	[[callop("ATOMIC_EXCHANGE")]] int32 atomic_exchange(uint32 loc, int32 desired);
	[[callop("ATOMIC_COMPARE_EXCHANGE")]] int32 atomic_compare_exchange(uint32 loc, int32 expected, int32 desired);
	[[callop("ATOMIC_FETCH_ADD")]] int32 atomic_fetch_add(uint32 loc, int32 val);
	[[callop("ATOMIC_FETCH_SUB")]] int32 atomic_fetch_sub(uint32 loc, int32 val);
	[[callop("ATOMIC_FETCH_AND")]] int32 atomic_fetch_and(uint32 loc, int32 val);
	[[callop("ATOMIC_FETCH_OR")]] int32 atomic_fetch_or(uint32 loc, int32 val);
	[[callop("ATOMIC_FETCH_XOR")]] int32 atomic_fetch_xor(uint32 loc, int32 val);
	[[callop("ATOMIC_FETCH_MIN")]] int32 atomic_fetch_min(uint32 loc, int32 val);
	[[callop("ATOMIC_FETCH_MAX")]] int32 atomic_fetch_max(uint32 loc, int32 val);
};
template<>
struct [[builtin("buffer")]] Buffer<uint32, BufferFlags::ReadWrite> {
	using ElementType = uint32;

	[[callop("BUFFER_READ")]] uint32 load(uint32 loc);
	[[callop("BUFFER_WRITE")]] void store(uint32 loc, uint32 value);
	[[callop("ATOMIC_EXCHANGE")]] uint32 atomic_exchange(uint32 loc, uint32 desired);
	[[callop("ATOMIC_COMPARE_EXCHANGE")]] uint32 atomic_compare_exchange(uint32 loc, uint32 expected, uint32 desired);
	[[callop("ATOMIC_FETCH_ADD")]] uint32 atomic_fetch_add(uint32 loc, uint32 val);
	[[callop("ATOMIC_FETCH_SUB")]] uint32 atomic_fetch_sub(uint32 loc, uint32 val);
	[[callop("ATOMIC_FETCH_AND")]] uint32 atomic_fetch_and(uint32 loc, uint32 val);
	[[callop("ATOMIC_FETCH_OR")]] uint32 atomic_fetch_or(uint32 loc, uint32 val);
	[[callop("ATOMIC_FETCH_XOR")]] uint32 atomic_fetch_xor(uint32 loc, uint32 val);
	[[callop("ATOMIC_FETCH_MIN")]] uint32 atomic_fetch_min(uint32 loc, uint32 val);
	[[callop("ATOMIC_FETCH_MAX")]] uint32 atomic_fetch_max(uint32 loc, uint32 val);
};
template<>
struct [[builtin("buffer")]] Buffer<float, BufferFlags::ReadWrite> {
	using ElementType = float;

	[[callop("BUFFER_READ")]] float load(uint32 loc);
	[[callop("BUFFER_WRITE")]] void store(uint32 loc, float value);
	[[callop("ATOMIC_EXCHANGE")]] float atomic_exchange(uint32 loc, float desired);
	[[callop("ATOMIC_COMPARE_EXCHANGE")]] float atomic_compare_exchange(uint32 loc, float expected, float desired);
	////////// Atomic float operation is risky, cull it currently
	// [[callop("ATOMIC_FETCH_ADD")]] float atomic_fetch_add(uint32 loc, float val);
	// [[callop("ATOMIC_FETCH_SUB")]] float atomic_fetch_sub(uint32 loc, float val);
	// [[callop("ATOMIC_FETCH_MIN")]] float atomic_fetch_min(uint32 loc, float val);
	// [[callop("ATOMIC_FETCH_MAX")]] float atomic_fetch_max(uint32 loc, float val);
};
template<uint32 cache_flags>
struct [[builtin("buffer")]] Buffer<void, cache_flags> {
	
	template<typename T = uint>
	[[callop("BYTE_BUFFER_READ")]] T Load(uint32 byte_index);
	
	template<typename T = uint>
	[[callop("BYTE_BUFFER_WRITE")]] void Store(uint32 byte_index, const T& val) requires((cache_flags & BufferFlags::ReadWrite) != 0);

	[[callop("BYTE_BUFFER_LOAD")]] uint Load(uint byte_index);
	[[callop("BYTE_BUFFER_LOAD2")]] uint2 Load2(uint byte_index);
	[[callop("BYTE_BUFFER_LOAD3")]] uint3 Load3(uint byte_index);
	[[callop("BYTE_BUFFER_LOAD4")]] uint4 Load4(uint byte_index);

	[[callop("BYTE_BUFFER_STORE")]] void Store(uint byte_index, uint value) requires((cache_flags & BufferFlags::ReadWrite) != 0);
	[[callop("BYTE_BUFFER_STORE2")]] void Store2(uint byte_index, uint2 value) requires((cache_flags & BufferFlags::ReadWrite) != 0);
	[[callop("BYTE_BUFFER_STORE3")]] void Store3(uint byte_index, uint3 value) requires((cache_flags & BufferFlags::ReadWrite) != 0);
	[[callop("BYTE_BUFFER_STORE4")]] void Store4(uint byte_index, uint4 value) requires((cache_flags & BufferFlags::ReadWrite) != 0);
};

template<typename Type>
using RWBuffer = Buffer<Type, BufferFlags::ReadWrite>;

using ByteAddressBuffer = Buffer<void, BufferFlags::ReadOnly>;
using RWByteAddressBuffer = Buffer<void, BufferFlags::ReadWrite>;

}// namespace skr::shader