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

// pack to snorm
inline uint32_t pack_snorm8(const float4& v)
{
    const float4   sv = clamp(v, -1.0f, 1.0f);
    const uint32_t x  = static_cast<uint32_t>(sv.x * 127.0f);
    const uint32_t y  = static_cast<uint32_t>(sv.y * 127.0f);
    const uint32_t z  = static_cast<uint32_t>(sv.z * 127.0f);
    const uint32_t w  = static_cast<uint32_t>(sv.w * 127.0f);
    return (x & 0xff) |
           ((y & 0xff) << 8) |
           ((z & 0xff) << 16) |
           ((w & 0xff) << 24);
}
inline uint32_t pack_snorm8(const double4& v)
{
    const double4  sv = clamp(v, -1.0, 1.0);
    const uint32_t x  = static_cast<uint32_t>(sv.x * 127.0);
    const uint32_t y  = static_cast<uint32_t>(sv.y * 127.0);
    const uint32_t z  = static_cast<uint32_t>(sv.z * 127.0);
    const uint32_t w  = static_cast<uint32_t>(sv.w * 127.0);
    return (x & 0xff) |
           ((y & 0xff) << 8) |
           ((z & 0xff) << 16) |
           ((w & 0xff) << 24);
}

// pack to unorm
inline uint32_t pack_unorm8(const float4& v)
{
    const float4   sv = clamp(v, 0.0f, 1.0f);
    const uint32_t x  = static_cast<uint32_t>(sv.x * 255.0f);
    const uint32_t y  = static_cast<uint32_t>(sv.y * 255.0f);
    const uint32_t z  = static_cast<uint32_t>(sv.z * 255.0f);
    const uint32_t w  = static_cast<uint32_t>(sv.w * 255.0f);
    return (x & 0xff) |
           ((y & 0xff) << 8) |
           ((z & 0xff) << 16) |
           ((w & 0xff) << 24);
}
inline uint32_t pack_unorm8(const double4& v)
{
    const double4  sv = clamp(v, 0.0, 1.0);
    const uint32_t x  = static_cast<uint32_t>(sv.x * 255.0);
    const uint32_t y  = static_cast<uint32_t>(sv.y * 255.0);
    const uint32_t z  = static_cast<uint32_t>(sv.z * 255.0);
    const uint32_t w  = static_cast<uint32_t>(sv.w * 255.0);
    return (x & 0xff) |
           ((y & 0xff) << 8) |
           ((z & 0xff) << 16) |
           ((w & 0xff) << 24);
}

} // namespace math
} // namespace skr