#pragma once
#include "attributes.hxx" // IWYU pragma: export
#include "system_values.hxx" // IWYU pragma: export
#include "type_traits.hxx" // IWYU pragma: export
#include "numeric.hxx" // IWYU pragma: export

#include "types.hxx" // IWYU pragma: export
#include "resources.hxx" // IWYU pragma: export

#include "intrinsics.hxx" // IWYU pragma: export
#include "raytracing.hxx" // IWYU pragma: export

template<typename Resource, typename T>
static void Store2D(Resource& r, uint32 row_pitch, uint2 pos, T val) {
	using ResourceType = cppsl::remove_cvref_t<Resource>;
	if constexpr (cppsl::is_same_v<ResourceType, Buffer<T>>)
		r.store(pos.x + pos.y * row_pitch, val);
	else if constexpr (cppsl::is_same_v<ResourceType, Texture2D<scalar_type<T>>>)
		r.store(pos, val);
}

template<concepts::primitive T>
constexpr void swap(T& l, T& r) {
	T tmp = l;
	l = r;
	r = tmp;
}
