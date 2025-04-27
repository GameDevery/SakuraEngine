#pragma once
#include "./rtm_conv.hpp"
#include "../gen/misc/quat.hpp"

namespace skr
{
inline namespace math
{
// is parallel
inline bool is_parallel(const float3& normal_a, const float3& normal_b, float threshold = 0.999845f)
{
    return abs(dot(normal_a, normal_b)) >= threshold;
}
inline bool is_parallel(const double3& normal_a, const double3& normal_b, double threshold = 0.999845)
{
    return abs(dot(normal_a, normal_b)) >= threshold;
}

// is coincident
inline bool is_coincident(const float3& normal_a, const float3& normal_b, float threshold = 0.999845f)
{
    return dot(normal_a, normal_b) >= threshold;
}
inline bool is_coincident(const double3& normal_a, const double3& normal_b, double threshold = 0.999845f)
{
    return dot(normal_a, normal_b) >= threshold;
}

// is orthogonal
inline bool is_orthogonal(const float3& normal_a, const float3& normal_b, float threshold = 0.017455f)
{
    return abs(dot(normal_a, normal_b)) <= threshold;
}
inline bool is_orthogonal(const double3& normal_a, const double3& normal_b, double threshold = 0.017455)
{
    return abs(dot(normal_a, normal_b)) <= threshold;
}

}
}