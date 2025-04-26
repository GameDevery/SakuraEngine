#pragma once
// skr types
#include "../gen/gen_math.hpp"

// rtm types
#include <rtm/quatf.h>
#include <rtm/quatd.h>
#include <rtm/vector4f.h>
#include <rtm/vector4d.h>
#include <rtm/qvvf.h>
#include <rtm/qvvd.h>
#include <rtm/matrix3x3f.h>
#include <rtm/matrix3x3d.h>
#include <rtm/matrix3x4f.h>
#include <rtm/matrix3x4d.h>
#include <rtm/matrix4x4f.h>
#include <rtm/matrix4x4f.h>

namespace skr
{
inline namespace math
{
template <typename T>
struct RtmConvert {
};

// vector2
template <>
struct RtmConvert<float2> {
    inline static rtm::vector4f to_rtm(const float2& v)
    {
        return rtm::vector_load2(&v.x);
    }
    inline static void store(const rtm::vector4f& v, float2& out)
    {
        rtm::vector_store2(v, &out.x);
    }
    inline static float2 from_rtm(const rtm::vector4f& v)
    {
        float2 result;
        store(v, result);
        return result;
    }
    
};
template <>
struct RtmConvert<double2> {
    inline static rtm::vector4d to_rtm(const double2& v)
    {
        return rtm::vector_load2(&v.x);
    }
    inline static void store(const rtm::vector4d& v, double2& out)
    {
        rtm::vector_store2(v, &out.x);
    }
    inline static double2 from_rtm(const rtm::vector4d& v)
    {
        double2 result;
        store(v, result);
        return result;
    }
};

// vector3
template <>
struct RtmConvert<float3> {
    inline static rtm::vector4f to_rtm(const float3& v)
    {
        return rtm::vector_load3(&v.x);
    }
    inline static void store(const rtm::vector4f& v, float3& out)
    {
        rtm::vector_store3(v, &out.x);
    }
    inline static float3 from_rtm(const rtm::vector4f& v)
    {
        float3 result;
        store(v, result);
        return result;
    }
};
template <>
struct RtmConvert<double3> {
    inline static rtm::vector4d to_rtm(const double3& v)
    {
        return rtm::vector_load3(&v.x);
    }
    inline static void store(const rtm::vector4d& v, double3& out)
    {
        rtm::vector_store3(v, &out.x);
    }
    inline static double3 from_rtm(const rtm::vector4d& v)
    {
        double3 result;
        store(v, result);
        return result;
    }
};

// vector4
template <>
struct RtmConvert<float4> {
    inline static rtm::vector4f to_rtm(const float4& v)
    {
        return rtm::vector_load(&v.x);
    }
    inline static void store(const rtm::vector4f& v, float4& out)
    {
        rtm::vector_store(v, &out.x);
    }
    inline static float4 from_rtm(const rtm::vector4f& v)
    {
        float4 result;
        store(v, result);
        return result;
    }
};
template <>
struct RtmConvert<double4> {
    inline static rtm::vector4d to_rtm(const double4& v)
    {
        return rtm::vector_load(&v.x);
    }
    inline static void store(const rtm::vector4d& v, double4& out)
    {
        rtm::vector_store(v, &out.x);
    }
    inline static double4 from_rtm(const rtm::vector4d& v)
    {
        double4 result;
        store(v, result);   
        return result;
    }
};

// quat
template <>
struct RtmConvert<QuatF> {
    inline static rtm::quatf to_rtm(const QuatF& q)
    {
        return rtm::quat_load(&q.x);
    }
    inline static void store(const rtm::quatf& q, QuatF& out)
    {
        rtm::quat_store(q, &out.x);
    }
    inline static QuatF from_rtm(const rtm::quatf& q)
    {
        QuatF result;
        store(q, result);
        return result;
    }
};
template <>
struct RtmConvert<QuatD> {
    inline static rtm::quatd to_rtm(const QuatD& q)
    {
        return rtm::quat_load(&q.x);
    }
    inline static void store(const rtm::quatd& q, QuatD& out)
    {
        rtm::quat_store(q, &out.x);
    }
    inline static QuatD from_rtm(const rtm::quatd& q)
    {
        QuatD result;
        store(q, result);
        return result;
    }
};
} // namespace math
} // namespace skr