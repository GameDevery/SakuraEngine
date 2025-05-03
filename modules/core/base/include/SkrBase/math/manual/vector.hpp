#pragma once
#include "./rtm_conv.hpp"
#include "../gen/misc/quat.hpp"

namespace skr
{
inline namespace math
{
// max
inline float max(float a, float b)
{
    return rtm::scalar_max(a, b);
}
inline float2 max(const float2& a, const float2& b)
{
    return RtmConvert<float2>::from_rtm(
        rtm::vector_max(
            RtmConvert<float2>::to_rtm(a),
            RtmConvert<float2>::to_rtm(b)
        )
    );
}
inline float3 max(const float3& a, const float3& b)
{
    return RtmConvert<float3>::from_rtm(
        rtm::vector_max(
            RtmConvert<float3>::to_rtm(a),
            RtmConvert<float3>::to_rtm(b)
        )
    );
}
inline float4 max(const float4& a, const float4& b)
{
    return RtmConvert<float4>::from_rtm(
        rtm::vector_max(
            RtmConvert<float4>::to_rtm(a),
            RtmConvert<float4>::to_rtm(b)
        )
    );
}
inline double max(double a, double b)
{
    return rtm::scalar_max(a, b);
}
inline double2 max(const double2& a, const double2& b)
{
    return RtmConvert<double2>::from_rtm(
        rtm::vector_max(
            RtmConvert<double2>::to_rtm(a),
            RtmConvert<double2>::to_rtm(b)
        )
    );
}
inline double3 max(const double3& a, const double3& b)
{
    return RtmConvert<double3>::from_rtm(
        rtm::vector_max(
            RtmConvert<double3>::to_rtm(a),
            RtmConvert<double3>::to_rtm(b)
        )
    );
}
inline double4 max(const double4& a, const double4& b)
{
    return RtmConvert<double4>::from_rtm(
        rtm::vector_max(
            RtmConvert<double4>::to_rtm(a),
            RtmConvert<double4>::to_rtm(b)
        )
    );
}

// min
inline float min(float a, float b)
{
    return rtm::scalar_min(a, b);
}
inline float2 min(const float2& a, const float2& b)
{
    return RtmConvert<float2>::from_rtm(
        rtm::vector_min(
            RtmConvert<float2>::to_rtm(a),
            RtmConvert<float2>::to_rtm(b)
        )
    );
}
inline float3 min(const float3& a, const float3& b)
{
    return RtmConvert<float3>::from_rtm(
        rtm::vector_min(
            RtmConvert<float3>::to_rtm(a),
            RtmConvert<float3>::to_rtm(b)
        )
    );
}
inline float4 min(const float4& a, const float4& b)
{
    return RtmConvert<float4>::from_rtm(
        rtm::vector_min(
            RtmConvert<float4>::to_rtm(a),
            RtmConvert<float4>::to_rtm(b)
        )
    );
}
inline double min(double a, double b)
{
    return rtm::scalar_min(a, b);
}
inline double2 min(const double2& a, const double2& b)
{
    return RtmConvert<double2>::from_rtm(
        rtm::vector_min(
            RtmConvert<double2>::to_rtm(a),
            RtmConvert<double2>::to_rtm(b)
        )
    );
}
inline double3 min(const double3& a, const double3& b)
{
    return RtmConvert<double3>::from_rtm(
        rtm::vector_min(
            RtmConvert<double3>::to_rtm(a),
            RtmConvert<double3>::to_rtm(b)
        )
    );
}
inline double4 min(const double4& a, const double4& b)
{
    return RtmConvert<double4>::from_rtm(
        rtm::vector_min(
            RtmConvert<double4>::to_rtm(a),
            RtmConvert<double4>::to_rtm(b)
        )
    );
}

// abs
inline float abs(float v)
{
    return rtm::scalar_abs(v);
}
inline float2 abs(const float2& v)
{
    return RtmConvert<float2>::from_rtm(
        rtm::vector_abs(
            RtmConvert<float2>::to_rtm(v)
        )
    );
}
inline float3 abs(const float3& v)
{
    return RtmConvert<float3>::from_rtm(
        rtm::vector_abs(
            RtmConvert<float3>::to_rtm(v)
        )
    );
}
inline float4 abs(const float4& v)
{
    return RtmConvert<float4>::from_rtm(
        rtm::vector_abs(
            RtmConvert<float4>::to_rtm(v)
        )
    );
}
inline double abs(double v)
{
    return rtm::scalar_abs(v);
}
inline double2 abs(const double2& v)
{
    return RtmConvert<double2>::from_rtm(
        rtm::vector_abs(
            RtmConvert<double2>::to_rtm(v)
        )
    );
}
inline double3 abs(const double3& v)
{
    return RtmConvert<double3>::from_rtm(
        rtm::vector_abs(
            RtmConvert<double3>::to_rtm(v)
        )
    );
}
inline double4 abs(const double4& v)
{
    return RtmConvert<double4>::from_rtm(
        rtm::vector_abs(
            RtmConvert<double4>::to_rtm(v)
        )
    );
}

// rcp
inline float rcp(float v)
{
    return rtm::scalar_reciprocal(v);
}
inline float2 rcp(const float2& v)
{
    return RtmConvert<float2>::from_rtm(
        rtm::vector_reciprocal(
            RtmConvert<float2>::to_rtm(v)
        )
    );
}
inline float3 rcp(const float3& v)
{
    return RtmConvert<float3>::from_rtm(
        rtm::vector_reciprocal(
            RtmConvert<float3>::to_rtm(v)
        )
    );
}
inline float4 rcp(const float4& v)
{
    return RtmConvert<float4>::from_rtm(
        rtm::vector_reciprocal(
            RtmConvert<float4>::to_rtm(v)
        )
    );
}
inline double rcp(double v)
{
    return rtm::scalar_reciprocal(v);
}
inline double2 rcp(const double2& v)
{
    return RtmConvert<double2>::from_rtm(
        rtm::vector_reciprocal(
            RtmConvert<double2>::to_rtm(v)
        )
    );
}
inline double3 rcp(const double3& v)
{
    return RtmConvert<double3>::from_rtm(
        rtm::vector_reciprocal(
            RtmConvert<double3>::to_rtm(v)
        )
    );
}
inline double4 rcp(const double4& v)
{
    return RtmConvert<double4>::from_rtm(
        rtm::vector_reciprocal(
            RtmConvert<double4>::to_rtm(v)
        )
    );
}

// sqrt
inline float sqrt(float v)
{
    return rtm::scalar_sqrt(v);
}
inline float2 sqrt(const float2& v)
{
    return RtmConvert<float2>::from_rtm(
        rtm::vector_sqrt(
            RtmConvert<float2>::to_rtm(v)
        )
    );
}
inline float3 sqrt(const float3& v)
{
    return RtmConvert<float3>::from_rtm(
        rtm::vector_sqrt(
            RtmConvert<float3>::to_rtm(v)
        )
    );
}
inline float4 sqrt(const float4& v)
{
    return RtmConvert<float4>::from_rtm(
        rtm::vector_sqrt(
            RtmConvert<float4>::to_rtm(v)
        )
    );
}
inline double sqrt(double v)
{
    return rtm::scalar_sqrt(v);
}
inline double2 sqrt(const double2& v)
{
    return RtmConvert<double2>::from_rtm(
        rtm::vector_sqrt(
            RtmConvert<double2>::to_rtm(v)
        )
    );
}
inline double3 sqrt(const double3& v)
{
    return RtmConvert<double3>::from_rtm(
        rtm::vector_sqrt(
            RtmConvert<double3>::to_rtm(v)
        )
    );
}
inline double4 sqrt(const double4& v)
{
    return RtmConvert<double4>::from_rtm(
        rtm::vector_sqrt(
            RtmConvert<double4>::to_rtm(v)
        )
    );
}

// rsqrt
inline float rsqrt(float v)
{
    return rtm::scalar_sqrt_reciprocal(v);
}
inline float2 rsqrt(const float2& v)
{
    return RtmConvert<float2>::from_rtm(
        rtm::vector_sqrt_reciprocal(
            RtmConvert<float2>::to_rtm(v)
        )
    );
}
inline float3 rsqrt(const float3& v)
{
    return RtmConvert<float3>::from_rtm(
        rtm::vector_sqrt_reciprocal(
            RtmConvert<float3>::to_rtm(v)
        )
    );
}
inline float4 rsqrt(const float4& v)
{
    return RtmConvert<float4>::from_rtm(
        rtm::vector_sqrt_reciprocal(
            RtmConvert<float4>::to_rtm(v)
        )
    );
}
inline double rsqrt(double v)
{
    return rtm::scalar_sqrt_reciprocal(v);
}
inline double2 rsqrt(const double2& v)
{
    return RtmConvert<double2>::from_rtm(
        rtm::vector_sqrt_reciprocal(
            RtmConvert<double2>::to_rtm(v)
        )
    );
}
inline double3 rsqrt(const double3& v)
{
    return RtmConvert<double3>::from_rtm(
        rtm::vector_sqrt_reciprocal(
            RtmConvert<double3>::to_rtm(v)
        )
    );
}
inline double4 rsqrt(const double4& v)
{
    return RtmConvert<double4>::from_rtm(
        rtm::vector_sqrt_reciprocal(
            RtmConvert<double4>::to_rtm(v)
        )
    );
}

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