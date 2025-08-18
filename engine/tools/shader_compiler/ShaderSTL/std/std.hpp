#pragma once
#include "attributes.hpp" // IWYU pragma: export
#include "type_traits.hpp" // IWYU pragma: export
#include "numeric.hpp" // IWYU pragma: export

#include "types.hpp" // IWYU pragma: export
#include "resources.hpp" // IWYU pragma: export

#include "intrinsics.hpp" // IWYU pragma: export
#include "raytracing.hpp" // IWYU pragma: export

namespace skr::shader {

template<typename Resource, typename T>
static void store_2d(Resource& r, uint32 row_pitch, uint2 pos, T val) {
	using ResourceType = remove_cvref_t<Resource>;
	if constexpr (is_same_v<ResourceType, Buffer<T>>)
		r.store(pos.x + pos.y * row_pitch, val);
	else if constexpr (is_same_v<ResourceType, Texture2D<scalar_type<T>>>)
		r.store(pos, val);
}

template<concepts::primitive T>
constexpr void swap(T& l, T& r) {
	T tmp = l;
	l = r;
	r = tmp;
}

}// namespace skr::shader
