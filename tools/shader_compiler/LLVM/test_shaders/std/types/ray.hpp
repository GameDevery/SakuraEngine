#pragma once
#include "vec.hpp"
#include "array.hpp"
#include "./../numeric.hpp"

namespace skr::shader {
struct Ray {
	Ray() = default;
	Ray(const float3& origin, const float3& dir, float t_min = 0.0f, float t_max = 1e30f)
		: _origin{origin.x, origin.y, origin.z}, t_min(t_min), _dir{dir.x, dir.y, dir.z}, t_max(t_max)
	{

	}
	[[nodiscard, noignore]] float3 origin() const {
		return float3(_origin[0], _origin[1], _origin[2]);
	}
	[[nodiscard, noignore]] float3 dir() const {
		return float3(_dir[0], _dir[1], _dir[2]);
	}
	[[nodiscard, noignore]] float tmin() const { return t_min; }
	[[nodiscard, noignore]] float tmax() const { return t_max; }

	Array<float, 3> _origin;
	float t_min = 0.0f;
	Array<float, 3> _dir;
	float t_max = 1e30f;
};

enum struct HitType : uint32
{
    Miss = 0,
    HitTriangle = 1,
    HitProcedural = 2
};

struct CommittedHit {
	uint32 inst;
	uint32 prim;
	float2 bary;
	HitType hit_type;
	float ray_t;
	[[nodiscard, noignore]] bool miss() const {
		return hit_type == HitType::Miss;
	}
	[[nodiscard, noignore]] bool hit_triangle() const {
		return hit_type == HitType::HitTriangle;
	}
	[[nodiscard, noignore]] bool hit_procedural() const {
		return hit_type == HitType::HitProcedural;
	}
	template<concepts::float_family T>
	T interpolate(const T& a, const T& b, const T& c) {
		return T(1.0f - bary.x - bary.y) * a + T(bary.x) * b + T(bary.y) * c;
	}
};

struct TriangleHit {
	uint32 inst;
	uint32 prim;
	float2 bary;
	float ray_t;
	[[nodiscard, noignore]] bool miss() const {
		return inst == max_uint32;
	}
	[[nodiscard, noignore]] bool hitted() const {
		return inst != max_uint32;
	}
	template<concepts::float_family T>
	T interpolate(const T& a, const T& b, const T& c) {
		return T(1.0f - bary.x - bary.y) * a + T(bary.x) * b + T(bary.y) * c;
	}
};
template<concepts::float_family T>
T interpolate(float2 bary, const T& a, const T& b, const T& c) {
	return T(1.0f - bary.x - bary.y) * a + T(bary.x) * b + T(bary.y) * c;
}

struct ProceduralHit {
	uint32 inst;
	uint32 prim;
};
}// namespace skr::shader