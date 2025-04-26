#pragma once
#include "./rtm_conv.hpp"
#include "../gen/misc/quat.hpp"

namespace skr
{
inline namespace math
{
// vector3 mul operator
inline float3x3 operator*(const float3x3& lhs, const float3x3& rhs)
{
    const auto rtm_lhs    = RtmConvert<float3x3>::to_rtm(lhs);
    const auto rtm_rhs    = RtmConvert<float3x3>::to_rtm(rhs);
    const auto rtm_result = rtm::matrix_mul(rtm_lhs, rtm_rhs);
    return {
        RtmConvert<float3>::from_rtm(rtm_result.x_axis),
        RtmConvert<float3>::from_rtm(rtm_result.y_axis),
        RtmConvert<float3>::from_rtm(rtm_result.z_axis)
    };
}
inline float3x3& float3x3::operator*=(const float3x3& rhs)
{
    const auto rtm_lhs    = RtmConvert<float3x3>::to_rtm(*this);
    const auto rtm_rhs    = RtmConvert<float3x3>::to_rtm(rhs);
    const auto rtm_result = rtm::matrix_mul(rtm_lhs, rtm_rhs);
    RtmConvert<float3>::store(rtm_result.x_axis, axis_x);
    RtmConvert<float3>::store(rtm_result.y_axis, axis_y);
    RtmConvert<float3>::store(rtm_result.z_axis, axis_z);
    return *this;
}
inline float3 operator*(const float3& lhs, const float3x3& rhs)
{
    const auto rtm_lhs    = RtmConvert<float3>::to_rtm(lhs);
    const auto rtm_rhs    = RtmConvert<float3x3>::to_rtm(rhs);
    const auto rtm_result = rtm::matrix_mul_vector3(rtm_lhs, rtm_rhs);
    return RtmConvert<float3>::from_rtm(rtm_result);
}
inline double3x3 operator*(const double3x3& lhs, const double3x3& rhs)
{
    const auto rtm_lhs    = RtmConvert<double3x3>::to_rtm(lhs);
    const auto rtm_rhs    = RtmConvert<double3x3>::to_rtm(rhs);
    const auto rtm_result = rtm::matrix_mul(rtm_lhs, rtm_rhs);
    return {
        RtmConvert<double3>::from_rtm(rtm_result.x_axis),
        RtmConvert<double3>::from_rtm(rtm_result.y_axis),
        RtmConvert<double3>::from_rtm(rtm_result.z_axis)
    };
}
inline double3x3& double3x3::operator*=(const double3x3& rhs)
{
    const auto rtm_lhs    = RtmConvert<double3x3>::to_rtm(*this);
    const auto rtm_rhs    = RtmConvert<double3x3>::to_rtm(rhs);
    const auto rtm_result = rtm::matrix_mul(rtm_lhs, rtm_rhs);
    RtmConvert<double3>::store(rtm_result.x_axis, axis_x);
    RtmConvert<double3>::store(rtm_result.y_axis, axis_y);
    RtmConvert<double3>::store(rtm_result.z_axis, axis_z);
    return *this;
}
inline double3 operator*(const double3& lhs, const double3x3& rhs)
{
    const auto rtm_lhs    = RtmConvert<double3>::to_rtm(lhs);
    const auto rtm_rhs    = RtmConvert<double3x3>::to_rtm(rhs);
    const auto rtm_result = rtm::matrix_mul_vector3(rtm_lhs, rtm_rhs);
    return RtmConvert<double3>::from_rtm(rtm_result);
}

// vector4 mul operator
inline float4x4 operator*(const float4x4& lhs, const float4x4& rhs)
{
    const auto rtm_lhs    = RtmConvert<float4x4>::to_rtm(lhs);
    const auto rtm_rhs    = RtmConvert<float4x4>::to_rtm(rhs);
    const auto rtm_result = rtm::matrix_mul(rtm_lhs, rtm_rhs);
    return {
        RtmConvert<float4>::from_rtm(rtm_result.x_axis),
        RtmConvert<float4>::from_rtm(rtm_result.y_axis),
        RtmConvert<float4>::from_rtm(rtm_result.z_axis),
        RtmConvert<float4>::from_rtm(rtm_result.w_axis)
    };
}
inline float4x4& float4x4::operator*=(const float4x4& rhs)
{
    const auto rtm_lhs    = RtmConvert<float4x4>::to_rtm(*this);
    const auto rtm_rhs    = RtmConvert<float4x4>::to_rtm(rhs);
    const auto rtm_result = rtm::matrix_mul(rtm_lhs, rtm_rhs);
    RtmConvert<float4>::store(rtm_result.x_axis, axis_x);
    RtmConvert<float4>::store(rtm_result.y_axis, axis_y);
    RtmConvert<float4>::store(rtm_result.z_axis, axis_z);
    RtmConvert<float4>::store(rtm_result.w_axis, axis_w);
    return *this;
}
inline float4 operator*(const float4& lhs, const float4x4& rhs)
{
    const auto rtm_lhs    = RtmConvert<float4>::to_rtm(lhs);
    const auto rtm_rhs    = RtmConvert<float4x4>::to_rtm(rhs);
    const auto rtm_result = rtm::matrix_mul_vector(rtm_lhs, rtm_rhs);
    return RtmConvert<float4>::from_rtm(rtm_result);
}
inline double4x4 operator*(const double4x4& lhs, const double4x4& rhs)
{
    const auto rtm_lhs    = RtmConvert<double4x4>::to_rtm(lhs);
    const auto rtm_rhs    = RtmConvert<double4x4>::to_rtm(rhs);
    const auto rtm_result = rtm::matrix_mul(rtm_lhs, rtm_rhs);
    return {
        RtmConvert<double4>::from_rtm(rtm_result.x_axis),
        RtmConvert<double4>::from_rtm(rtm_result.y_axis),
        RtmConvert<double4>::from_rtm(rtm_result.z_axis),
        RtmConvert<double4>::from_rtm(rtm_result.w_axis)
    };
}
inline double4x4& double4x4::operator*=(const double4x4& rhs)
{
    const auto rtm_lhs    = RtmConvert<double4x4>::to_rtm(*this);
    const auto rtm_rhs    = RtmConvert<double4x4>::to_rtm(rhs);
    const auto rtm_result = rtm::matrix_mul(rtm_lhs, rtm_rhs);
    RtmConvert<double4>::store(rtm_result.x_axis, axis_x);
    RtmConvert<double4>::store(rtm_result.y_axis, axis_y);
    RtmConvert<double4>::store(rtm_result.z_axis, axis_z);
    RtmConvert<double4>::store(rtm_result.w_axis, axis_w);
    return *this;
}
inline double4 operator*(const double4& lhs, const double4x4& rhs)
{
    const auto rtm_lhs    = RtmConvert<double4>::to_rtm(lhs);
    const auto rtm_rhs    = RtmConvert<double4x4>::to_rtm(rhs);
    const auto rtm_result = rtm::matrix_mul_vector(rtm_lhs, rtm_rhs);
    return RtmConvert<double4>::from_rtm(rtm_result);
}

// vector3 mul free function
inline float3x3 mul(const float3x3& lhs, const float3x3& rhs)
{
    return lhs * rhs;
}
inline float3 mul(const float3& lhs, const float3x3& rhs)
{
    return lhs * rhs;
}
inline double3x3 mul(const double3x3& lhs, const double3x3& rhs)
{
    return lhs * rhs;
}
inline double3 mul(const double3& lhs, const double3x3& rhs)
{
    return lhs * rhs;
}

// vector4 mul free function
inline float4x4 mul(const float4x4& lhs, const float4x4& rhs)
{
    return lhs * rhs;
}
inline float4 mul(const float4& lhs, const float4x4& rhs)
{
    return lhs * rhs;
}
inline double4x4 mul(const double4x4& lhs, const double4x4& rhs)
{
    return lhs * rhs;
}
inline double4 mul(const double4& lhs, const double4x4& rhs)
{
    return lhs * rhs;
}

} // namespace math
} // namespace skr