//! *************************************************************************
//! **  This file is auto-generated by gen_math, do not edit it manually.  **
//! *************************************************************************

#pragma once
#include "float_vec.hpp"
#include "double_vec.hpp"
#include "bool_vec.hpp"
#include "int_vec.hpp"
#include "uint_vec.hpp"
#include "long_vec.hpp"
#include "ulong_vec.hpp"

namespace skr {
inline namespace math {
// compare operator for [float2]
inline bool2 operator==(const float2& lhs, const float2& rhs) { return {lhs.x == rhs.x, lhs.y == rhs.y}; }
inline bool2 operator!=(const float2& lhs, const float2& rhs) { return {lhs.x != rhs.x, lhs.y != rhs.y}; }
inline bool2 operator<(const float2& lhs, const float2& rhs) { return {lhs.x < rhs.x, lhs.y < rhs.y}; }
inline bool2 operator<=(const float2& lhs, const float2& rhs) { return {lhs.x <= rhs.x, lhs.y <= rhs.y}; }
inline bool2 operator>(const float2& lhs, const float2& rhs) { return {lhs.x > rhs.x, lhs.y > rhs.y}; }
inline bool2 operator>=(const float2& lhs, const float2& rhs) { return {lhs.x >= rhs.x, lhs.y >= rhs.y}; }

// compare operator for [float3]
inline bool3 operator==(const float3& lhs, const float3& rhs) { return {lhs.x == rhs.x, lhs.y == rhs.y, lhs.z == rhs.z}; }
inline bool3 operator!=(const float3& lhs, const float3& rhs) { return {lhs.x != rhs.x, lhs.y != rhs.y, lhs.z != rhs.z}; }
inline bool3 operator<(const float3& lhs, const float3& rhs) { return {lhs.x < rhs.x, lhs.y < rhs.y, lhs.z < rhs.z}; }
inline bool3 operator<=(const float3& lhs, const float3& rhs) { return {lhs.x <= rhs.x, lhs.y <= rhs.y, lhs.z <= rhs.z}; }
inline bool3 operator>(const float3& lhs, const float3& rhs) { return {lhs.x > rhs.x, lhs.y > rhs.y, lhs.z > rhs.z}; }
inline bool3 operator>=(const float3& lhs, const float3& rhs) { return {lhs.x >= rhs.x, lhs.y >= rhs.y, lhs.z >= rhs.z}; }

// compare operator for [float4]
inline bool4 operator==(const float4& lhs, const float4& rhs) { return {lhs.x == rhs.x, lhs.y == rhs.y, lhs.z == rhs.z, lhs.w == rhs.w}; }
inline bool4 operator!=(const float4& lhs, const float4& rhs) { return {lhs.x != rhs.x, lhs.y != rhs.y, lhs.z != rhs.z, lhs.w != rhs.w}; }
inline bool4 operator<(const float4& lhs, const float4& rhs) { return {lhs.x < rhs.x, lhs.y < rhs.y, lhs.z < rhs.z, lhs.w < rhs.w}; }
inline bool4 operator<=(const float4& lhs, const float4& rhs) { return {lhs.x <= rhs.x, lhs.y <= rhs.y, lhs.z <= rhs.z, lhs.w <= rhs.w}; }
inline bool4 operator>(const float4& lhs, const float4& rhs) { return {lhs.x > rhs.x, lhs.y > rhs.y, lhs.z > rhs.z, lhs.w > rhs.w}; }
inline bool4 operator>=(const float4& lhs, const float4& rhs) { return {lhs.x >= rhs.x, lhs.y >= rhs.y, lhs.z >= rhs.z, lhs.w >= rhs.w}; }

// compare operator for [double2]
inline bool2 operator==(const double2& lhs, const double2& rhs) { return {lhs.x == rhs.x, lhs.y == rhs.y}; }
inline bool2 operator!=(const double2& lhs, const double2& rhs) { return {lhs.x != rhs.x, lhs.y != rhs.y}; }
inline bool2 operator<(const double2& lhs, const double2& rhs) { return {lhs.x < rhs.x, lhs.y < rhs.y}; }
inline bool2 operator<=(const double2& lhs, const double2& rhs) { return {lhs.x <= rhs.x, lhs.y <= rhs.y}; }
inline bool2 operator>(const double2& lhs, const double2& rhs) { return {lhs.x > rhs.x, lhs.y > rhs.y}; }
inline bool2 operator>=(const double2& lhs, const double2& rhs) { return {lhs.x >= rhs.x, lhs.y >= rhs.y}; }

// compare operator for [double3]
inline bool3 operator==(const double3& lhs, const double3& rhs) { return {lhs.x == rhs.x, lhs.y == rhs.y, lhs.z == rhs.z}; }
inline bool3 operator!=(const double3& lhs, const double3& rhs) { return {lhs.x != rhs.x, lhs.y != rhs.y, lhs.z != rhs.z}; }
inline bool3 operator<(const double3& lhs, const double3& rhs) { return {lhs.x < rhs.x, lhs.y < rhs.y, lhs.z < rhs.z}; }
inline bool3 operator<=(const double3& lhs, const double3& rhs) { return {lhs.x <= rhs.x, lhs.y <= rhs.y, lhs.z <= rhs.z}; }
inline bool3 operator>(const double3& lhs, const double3& rhs) { return {lhs.x > rhs.x, lhs.y > rhs.y, lhs.z > rhs.z}; }
inline bool3 operator>=(const double3& lhs, const double3& rhs) { return {lhs.x >= rhs.x, lhs.y >= rhs.y, lhs.z >= rhs.z}; }

// compare operator for [double4]
inline bool4 operator==(const double4& lhs, const double4& rhs) { return {lhs.x == rhs.x, lhs.y == rhs.y, lhs.z == rhs.z, lhs.w == rhs.w}; }
inline bool4 operator!=(const double4& lhs, const double4& rhs) { return {lhs.x != rhs.x, lhs.y != rhs.y, lhs.z != rhs.z, lhs.w != rhs.w}; }
inline bool4 operator<(const double4& lhs, const double4& rhs) { return {lhs.x < rhs.x, lhs.y < rhs.y, lhs.z < rhs.z, lhs.w < rhs.w}; }
inline bool4 operator<=(const double4& lhs, const double4& rhs) { return {lhs.x <= rhs.x, lhs.y <= rhs.y, lhs.z <= rhs.z, lhs.w <= rhs.w}; }
inline bool4 operator>(const double4& lhs, const double4& rhs) { return {lhs.x > rhs.x, lhs.y > rhs.y, lhs.z > rhs.z, lhs.w > rhs.w}; }
inline bool4 operator>=(const double4& lhs, const double4& rhs) { return {lhs.x >= rhs.x, lhs.y >= rhs.y, lhs.z >= rhs.z, lhs.w >= rhs.w}; }

// compare operator for [int2]
inline bool2 operator==(const int2& lhs, const int2& rhs) { return {lhs.x == rhs.x, lhs.y == rhs.y}; }
inline bool2 operator!=(const int2& lhs, const int2& rhs) { return {lhs.x != rhs.x, lhs.y != rhs.y}; }
inline bool2 operator<(const int2& lhs, const int2& rhs) { return {lhs.x < rhs.x, lhs.y < rhs.y}; }
inline bool2 operator<=(const int2& lhs, const int2& rhs) { return {lhs.x <= rhs.x, lhs.y <= rhs.y}; }
inline bool2 operator>(const int2& lhs, const int2& rhs) { return {lhs.x > rhs.x, lhs.y > rhs.y}; }
inline bool2 operator>=(const int2& lhs, const int2& rhs) { return {lhs.x >= rhs.x, lhs.y >= rhs.y}; }

// compare operator for [int3]
inline bool3 operator==(const int3& lhs, const int3& rhs) { return {lhs.x == rhs.x, lhs.y == rhs.y, lhs.z == rhs.z}; }
inline bool3 operator!=(const int3& lhs, const int3& rhs) { return {lhs.x != rhs.x, lhs.y != rhs.y, lhs.z != rhs.z}; }
inline bool3 operator<(const int3& lhs, const int3& rhs) { return {lhs.x < rhs.x, lhs.y < rhs.y, lhs.z < rhs.z}; }
inline bool3 operator<=(const int3& lhs, const int3& rhs) { return {lhs.x <= rhs.x, lhs.y <= rhs.y, lhs.z <= rhs.z}; }
inline bool3 operator>(const int3& lhs, const int3& rhs) { return {lhs.x > rhs.x, lhs.y > rhs.y, lhs.z > rhs.z}; }
inline bool3 operator>=(const int3& lhs, const int3& rhs) { return {lhs.x >= rhs.x, lhs.y >= rhs.y, lhs.z >= rhs.z}; }

// compare operator for [int4]
inline bool4 operator==(const int4& lhs, const int4& rhs) { return {lhs.x == rhs.x, lhs.y == rhs.y, lhs.z == rhs.z, lhs.w == rhs.w}; }
inline bool4 operator!=(const int4& lhs, const int4& rhs) { return {lhs.x != rhs.x, lhs.y != rhs.y, lhs.z != rhs.z, lhs.w != rhs.w}; }
inline bool4 operator<(const int4& lhs, const int4& rhs) { return {lhs.x < rhs.x, lhs.y < rhs.y, lhs.z < rhs.z, lhs.w < rhs.w}; }
inline bool4 operator<=(const int4& lhs, const int4& rhs) { return {lhs.x <= rhs.x, lhs.y <= rhs.y, lhs.z <= rhs.z, lhs.w <= rhs.w}; }
inline bool4 operator>(const int4& lhs, const int4& rhs) { return {lhs.x > rhs.x, lhs.y > rhs.y, lhs.z > rhs.z, lhs.w > rhs.w}; }
inline bool4 operator>=(const int4& lhs, const int4& rhs) { return {lhs.x >= rhs.x, lhs.y >= rhs.y, lhs.z >= rhs.z, lhs.w >= rhs.w}; }

// compare operator for [uint2]
inline bool2 operator==(const uint2& lhs, const uint2& rhs) { return {lhs.x == rhs.x, lhs.y == rhs.y}; }
inline bool2 operator!=(const uint2& lhs, const uint2& rhs) { return {lhs.x != rhs.x, lhs.y != rhs.y}; }
inline bool2 operator<(const uint2& lhs, const uint2& rhs) { return {lhs.x < rhs.x, lhs.y < rhs.y}; }
inline bool2 operator<=(const uint2& lhs, const uint2& rhs) { return {lhs.x <= rhs.x, lhs.y <= rhs.y}; }
inline bool2 operator>(const uint2& lhs, const uint2& rhs) { return {lhs.x > rhs.x, lhs.y > rhs.y}; }
inline bool2 operator>=(const uint2& lhs, const uint2& rhs) { return {lhs.x >= rhs.x, lhs.y >= rhs.y}; }

// compare operator for [uint3]
inline bool3 operator==(const uint3& lhs, const uint3& rhs) { return {lhs.x == rhs.x, lhs.y == rhs.y, lhs.z == rhs.z}; }
inline bool3 operator!=(const uint3& lhs, const uint3& rhs) { return {lhs.x != rhs.x, lhs.y != rhs.y, lhs.z != rhs.z}; }
inline bool3 operator<(const uint3& lhs, const uint3& rhs) { return {lhs.x < rhs.x, lhs.y < rhs.y, lhs.z < rhs.z}; }
inline bool3 operator<=(const uint3& lhs, const uint3& rhs) { return {lhs.x <= rhs.x, lhs.y <= rhs.y, lhs.z <= rhs.z}; }
inline bool3 operator>(const uint3& lhs, const uint3& rhs) { return {lhs.x > rhs.x, lhs.y > rhs.y, lhs.z > rhs.z}; }
inline bool3 operator>=(const uint3& lhs, const uint3& rhs) { return {lhs.x >= rhs.x, lhs.y >= rhs.y, lhs.z >= rhs.z}; }

// compare operator for [uint4]
inline bool4 operator==(const uint4& lhs, const uint4& rhs) { return {lhs.x == rhs.x, lhs.y == rhs.y, lhs.z == rhs.z, lhs.w == rhs.w}; }
inline bool4 operator!=(const uint4& lhs, const uint4& rhs) { return {lhs.x != rhs.x, lhs.y != rhs.y, lhs.z != rhs.z, lhs.w != rhs.w}; }
inline bool4 operator<(const uint4& lhs, const uint4& rhs) { return {lhs.x < rhs.x, lhs.y < rhs.y, lhs.z < rhs.z, lhs.w < rhs.w}; }
inline bool4 operator<=(const uint4& lhs, const uint4& rhs) { return {lhs.x <= rhs.x, lhs.y <= rhs.y, lhs.z <= rhs.z, lhs.w <= rhs.w}; }
inline bool4 operator>(const uint4& lhs, const uint4& rhs) { return {lhs.x > rhs.x, lhs.y > rhs.y, lhs.z > rhs.z, lhs.w > rhs.w}; }
inline bool4 operator>=(const uint4& lhs, const uint4& rhs) { return {lhs.x >= rhs.x, lhs.y >= rhs.y, lhs.z >= rhs.z, lhs.w >= rhs.w}; }

// compare operator for [long2]
inline bool2 operator==(const long2& lhs, const long2& rhs) { return {lhs.x == rhs.x, lhs.y == rhs.y}; }
inline bool2 operator!=(const long2& lhs, const long2& rhs) { return {lhs.x != rhs.x, lhs.y != rhs.y}; }
inline bool2 operator<(const long2& lhs, const long2& rhs) { return {lhs.x < rhs.x, lhs.y < rhs.y}; }
inline bool2 operator<=(const long2& lhs, const long2& rhs) { return {lhs.x <= rhs.x, lhs.y <= rhs.y}; }
inline bool2 operator>(const long2& lhs, const long2& rhs) { return {lhs.x > rhs.x, lhs.y > rhs.y}; }
inline bool2 operator>=(const long2& lhs, const long2& rhs) { return {lhs.x >= rhs.x, lhs.y >= rhs.y}; }

// compare operator for [long3]
inline bool3 operator==(const long3& lhs, const long3& rhs) { return {lhs.x == rhs.x, lhs.y == rhs.y, lhs.z == rhs.z}; }
inline bool3 operator!=(const long3& lhs, const long3& rhs) { return {lhs.x != rhs.x, lhs.y != rhs.y, lhs.z != rhs.z}; }
inline bool3 operator<(const long3& lhs, const long3& rhs) { return {lhs.x < rhs.x, lhs.y < rhs.y, lhs.z < rhs.z}; }
inline bool3 operator<=(const long3& lhs, const long3& rhs) { return {lhs.x <= rhs.x, lhs.y <= rhs.y, lhs.z <= rhs.z}; }
inline bool3 operator>(const long3& lhs, const long3& rhs) { return {lhs.x > rhs.x, lhs.y > rhs.y, lhs.z > rhs.z}; }
inline bool3 operator>=(const long3& lhs, const long3& rhs) { return {lhs.x >= rhs.x, lhs.y >= rhs.y, lhs.z >= rhs.z}; }

// compare operator for [long4]
inline bool4 operator==(const long4& lhs, const long4& rhs) { return {lhs.x == rhs.x, lhs.y == rhs.y, lhs.z == rhs.z, lhs.w == rhs.w}; }
inline bool4 operator!=(const long4& lhs, const long4& rhs) { return {lhs.x != rhs.x, lhs.y != rhs.y, lhs.z != rhs.z, lhs.w != rhs.w}; }
inline bool4 operator<(const long4& lhs, const long4& rhs) { return {lhs.x < rhs.x, lhs.y < rhs.y, lhs.z < rhs.z, lhs.w < rhs.w}; }
inline bool4 operator<=(const long4& lhs, const long4& rhs) { return {lhs.x <= rhs.x, lhs.y <= rhs.y, lhs.z <= rhs.z, lhs.w <= rhs.w}; }
inline bool4 operator>(const long4& lhs, const long4& rhs) { return {lhs.x > rhs.x, lhs.y > rhs.y, lhs.z > rhs.z, lhs.w > rhs.w}; }
inline bool4 operator>=(const long4& lhs, const long4& rhs) { return {lhs.x >= rhs.x, lhs.y >= rhs.y, lhs.z >= rhs.z, lhs.w >= rhs.w}; }

// compare operator for [ulong2]
inline bool2 operator==(const ulong2& lhs, const ulong2& rhs) { return {lhs.x == rhs.x, lhs.y == rhs.y}; }
inline bool2 operator!=(const ulong2& lhs, const ulong2& rhs) { return {lhs.x != rhs.x, lhs.y != rhs.y}; }
inline bool2 operator<(const ulong2& lhs, const ulong2& rhs) { return {lhs.x < rhs.x, lhs.y < rhs.y}; }
inline bool2 operator<=(const ulong2& lhs, const ulong2& rhs) { return {lhs.x <= rhs.x, lhs.y <= rhs.y}; }
inline bool2 operator>(const ulong2& lhs, const ulong2& rhs) { return {lhs.x > rhs.x, lhs.y > rhs.y}; }
inline bool2 operator>=(const ulong2& lhs, const ulong2& rhs) { return {lhs.x >= rhs.x, lhs.y >= rhs.y}; }

// compare operator for [ulong3]
inline bool3 operator==(const ulong3& lhs, const ulong3& rhs) { return {lhs.x == rhs.x, lhs.y == rhs.y, lhs.z == rhs.z}; }
inline bool3 operator!=(const ulong3& lhs, const ulong3& rhs) { return {lhs.x != rhs.x, lhs.y != rhs.y, lhs.z != rhs.z}; }
inline bool3 operator<(const ulong3& lhs, const ulong3& rhs) { return {lhs.x < rhs.x, lhs.y < rhs.y, lhs.z < rhs.z}; }
inline bool3 operator<=(const ulong3& lhs, const ulong3& rhs) { return {lhs.x <= rhs.x, lhs.y <= rhs.y, lhs.z <= rhs.z}; }
inline bool3 operator>(const ulong3& lhs, const ulong3& rhs) { return {lhs.x > rhs.x, lhs.y > rhs.y, lhs.z > rhs.z}; }
inline bool3 operator>=(const ulong3& lhs, const ulong3& rhs) { return {lhs.x >= rhs.x, lhs.y >= rhs.y, lhs.z >= rhs.z}; }

// compare operator for [ulong4]
inline bool4 operator==(const ulong4& lhs, const ulong4& rhs) { return {lhs.x == rhs.x, lhs.y == rhs.y, lhs.z == rhs.z, lhs.w == rhs.w}; }
inline bool4 operator!=(const ulong4& lhs, const ulong4& rhs) { return {lhs.x != rhs.x, lhs.y != rhs.y, lhs.z != rhs.z, lhs.w != rhs.w}; }
inline bool4 operator<(const ulong4& lhs, const ulong4& rhs) { return {lhs.x < rhs.x, lhs.y < rhs.y, lhs.z < rhs.z, lhs.w < rhs.w}; }
inline bool4 operator<=(const ulong4& lhs, const ulong4& rhs) { return {lhs.x <= rhs.x, lhs.y <= rhs.y, lhs.z <= rhs.z, lhs.w <= rhs.w}; }
inline bool4 operator>(const ulong4& lhs, const ulong4& rhs) { return {lhs.x > rhs.x, lhs.y > rhs.y, lhs.z > rhs.z, lhs.w > rhs.w}; }
inline bool4 operator>=(const ulong4& lhs, const ulong4& rhs) { return {lhs.x >= rhs.x, lhs.y >= rhs.y, lhs.z >= rhs.z, lhs.w >= rhs.w}; }

// convert operator for [float2]
inline float2::float2(const double2& rhs) : x(static_cast<float>(rhs.x)), y(static_cast<float>(rhs.y)) {}
inline float2::float2(const bool2& rhs) : x(static_cast<float>(rhs.x)), y(static_cast<float>(rhs.y)) {}
inline float2::float2(const int2& rhs) : x(static_cast<float>(rhs.x)), y(static_cast<float>(rhs.y)) {}
inline float2::float2(const uint2& rhs) : x(static_cast<float>(rhs.x)), y(static_cast<float>(rhs.y)) {}
inline float2::float2(const long2& rhs) : x(static_cast<float>(rhs.x)), y(static_cast<float>(rhs.y)) {}
inline float2::float2(const ulong2& rhs) : x(static_cast<float>(rhs.x)), y(static_cast<float>(rhs.y)) {}

// convert operator for [float3]
inline float3::float3(const double3& rhs) : x(static_cast<float>(rhs.x)), y(static_cast<float>(rhs.y)), z(static_cast<float>(rhs.z)) {}
inline float3::float3(const bool3& rhs) : x(static_cast<float>(rhs.x)), y(static_cast<float>(rhs.y)), z(static_cast<float>(rhs.z)) {}
inline float3::float3(const int3& rhs) : x(static_cast<float>(rhs.x)), y(static_cast<float>(rhs.y)), z(static_cast<float>(rhs.z)) {}
inline float3::float3(const uint3& rhs) : x(static_cast<float>(rhs.x)), y(static_cast<float>(rhs.y)), z(static_cast<float>(rhs.z)) {}
inline float3::float3(const long3& rhs) : x(static_cast<float>(rhs.x)), y(static_cast<float>(rhs.y)), z(static_cast<float>(rhs.z)) {}
inline float3::float3(const ulong3& rhs) : x(static_cast<float>(rhs.x)), y(static_cast<float>(rhs.y)), z(static_cast<float>(rhs.z)) {}

// convert operator for [float4]
inline float4::float4(const double4& rhs) : x(static_cast<float>(rhs.x)), y(static_cast<float>(rhs.y)), z(static_cast<float>(rhs.z)), w(static_cast<float>(rhs.w)) {}
inline float4::float4(const bool4& rhs) : x(static_cast<float>(rhs.x)), y(static_cast<float>(rhs.y)), z(static_cast<float>(rhs.z)), w(static_cast<float>(rhs.w)) {}
inline float4::float4(const int4& rhs) : x(static_cast<float>(rhs.x)), y(static_cast<float>(rhs.y)), z(static_cast<float>(rhs.z)), w(static_cast<float>(rhs.w)) {}
inline float4::float4(const uint4& rhs) : x(static_cast<float>(rhs.x)), y(static_cast<float>(rhs.y)), z(static_cast<float>(rhs.z)), w(static_cast<float>(rhs.w)) {}
inline float4::float4(const long4& rhs) : x(static_cast<float>(rhs.x)), y(static_cast<float>(rhs.y)), z(static_cast<float>(rhs.z)), w(static_cast<float>(rhs.w)) {}
inline float4::float4(const ulong4& rhs) : x(static_cast<float>(rhs.x)), y(static_cast<float>(rhs.y)), z(static_cast<float>(rhs.z)), w(static_cast<float>(rhs.w)) {}

// convert operator for [double2]
inline double2::double2(const float2& rhs) : x(static_cast<double>(rhs.x)), y(static_cast<double>(rhs.y)) {}
inline double2::double2(const bool2& rhs) : x(static_cast<double>(rhs.x)), y(static_cast<double>(rhs.y)) {}
inline double2::double2(const int2& rhs) : x(static_cast<double>(rhs.x)), y(static_cast<double>(rhs.y)) {}
inline double2::double2(const uint2& rhs) : x(static_cast<double>(rhs.x)), y(static_cast<double>(rhs.y)) {}
inline double2::double2(const long2& rhs) : x(static_cast<double>(rhs.x)), y(static_cast<double>(rhs.y)) {}
inline double2::double2(const ulong2& rhs) : x(static_cast<double>(rhs.x)), y(static_cast<double>(rhs.y)) {}

// convert operator for [double3]
inline double3::double3(const float3& rhs) : x(static_cast<double>(rhs.x)), y(static_cast<double>(rhs.y)), z(static_cast<double>(rhs.z)) {}
inline double3::double3(const bool3& rhs) : x(static_cast<double>(rhs.x)), y(static_cast<double>(rhs.y)), z(static_cast<double>(rhs.z)) {}
inline double3::double3(const int3& rhs) : x(static_cast<double>(rhs.x)), y(static_cast<double>(rhs.y)), z(static_cast<double>(rhs.z)) {}
inline double3::double3(const uint3& rhs) : x(static_cast<double>(rhs.x)), y(static_cast<double>(rhs.y)), z(static_cast<double>(rhs.z)) {}
inline double3::double3(const long3& rhs) : x(static_cast<double>(rhs.x)), y(static_cast<double>(rhs.y)), z(static_cast<double>(rhs.z)) {}
inline double3::double3(const ulong3& rhs) : x(static_cast<double>(rhs.x)), y(static_cast<double>(rhs.y)), z(static_cast<double>(rhs.z)) {}

// convert operator for [double4]
inline double4::double4(const float4& rhs) : x(static_cast<double>(rhs.x)), y(static_cast<double>(rhs.y)), z(static_cast<double>(rhs.z)), w(static_cast<double>(rhs.w)) {}
inline double4::double4(const bool4& rhs) : x(static_cast<double>(rhs.x)), y(static_cast<double>(rhs.y)), z(static_cast<double>(rhs.z)), w(static_cast<double>(rhs.w)) {}
inline double4::double4(const int4& rhs) : x(static_cast<double>(rhs.x)), y(static_cast<double>(rhs.y)), z(static_cast<double>(rhs.z)), w(static_cast<double>(rhs.w)) {}
inline double4::double4(const uint4& rhs) : x(static_cast<double>(rhs.x)), y(static_cast<double>(rhs.y)), z(static_cast<double>(rhs.z)), w(static_cast<double>(rhs.w)) {}
inline double4::double4(const long4& rhs) : x(static_cast<double>(rhs.x)), y(static_cast<double>(rhs.y)), z(static_cast<double>(rhs.z)), w(static_cast<double>(rhs.w)) {}
inline double4::double4(const ulong4& rhs) : x(static_cast<double>(rhs.x)), y(static_cast<double>(rhs.y)), z(static_cast<double>(rhs.z)), w(static_cast<double>(rhs.w)) {}

// convert operator for [bool2]
inline bool2::bool2(const float2& rhs) : x(static_cast<bool>(rhs.x)), y(static_cast<bool>(rhs.y)) {}
inline bool2::bool2(const double2& rhs) : x(static_cast<bool>(rhs.x)), y(static_cast<bool>(rhs.y)) {}
inline bool2::bool2(const int2& rhs) : x(static_cast<bool>(rhs.x)), y(static_cast<bool>(rhs.y)) {}
inline bool2::bool2(const uint2& rhs) : x(static_cast<bool>(rhs.x)), y(static_cast<bool>(rhs.y)) {}
inline bool2::bool2(const long2& rhs) : x(static_cast<bool>(rhs.x)), y(static_cast<bool>(rhs.y)) {}
inline bool2::bool2(const ulong2& rhs) : x(static_cast<bool>(rhs.x)), y(static_cast<bool>(rhs.y)) {}

// convert operator for [bool3]
inline bool3::bool3(const float3& rhs) : x(static_cast<bool>(rhs.x)), y(static_cast<bool>(rhs.y)), z(static_cast<bool>(rhs.z)) {}
inline bool3::bool3(const double3& rhs) : x(static_cast<bool>(rhs.x)), y(static_cast<bool>(rhs.y)), z(static_cast<bool>(rhs.z)) {}
inline bool3::bool3(const int3& rhs) : x(static_cast<bool>(rhs.x)), y(static_cast<bool>(rhs.y)), z(static_cast<bool>(rhs.z)) {}
inline bool3::bool3(const uint3& rhs) : x(static_cast<bool>(rhs.x)), y(static_cast<bool>(rhs.y)), z(static_cast<bool>(rhs.z)) {}
inline bool3::bool3(const long3& rhs) : x(static_cast<bool>(rhs.x)), y(static_cast<bool>(rhs.y)), z(static_cast<bool>(rhs.z)) {}
inline bool3::bool3(const ulong3& rhs) : x(static_cast<bool>(rhs.x)), y(static_cast<bool>(rhs.y)), z(static_cast<bool>(rhs.z)) {}

// convert operator for [bool4]
inline bool4::bool4(const float4& rhs) : x(static_cast<bool>(rhs.x)), y(static_cast<bool>(rhs.y)), z(static_cast<bool>(rhs.z)), w(static_cast<bool>(rhs.w)) {}
inline bool4::bool4(const double4& rhs) : x(static_cast<bool>(rhs.x)), y(static_cast<bool>(rhs.y)), z(static_cast<bool>(rhs.z)), w(static_cast<bool>(rhs.w)) {}
inline bool4::bool4(const int4& rhs) : x(static_cast<bool>(rhs.x)), y(static_cast<bool>(rhs.y)), z(static_cast<bool>(rhs.z)), w(static_cast<bool>(rhs.w)) {}
inline bool4::bool4(const uint4& rhs) : x(static_cast<bool>(rhs.x)), y(static_cast<bool>(rhs.y)), z(static_cast<bool>(rhs.z)), w(static_cast<bool>(rhs.w)) {}
inline bool4::bool4(const long4& rhs) : x(static_cast<bool>(rhs.x)), y(static_cast<bool>(rhs.y)), z(static_cast<bool>(rhs.z)), w(static_cast<bool>(rhs.w)) {}
inline bool4::bool4(const ulong4& rhs) : x(static_cast<bool>(rhs.x)), y(static_cast<bool>(rhs.y)), z(static_cast<bool>(rhs.z)), w(static_cast<bool>(rhs.w)) {}

// convert operator for [int2]
inline int2::int2(const float2& rhs) : x(static_cast<int32_t>(rhs.x)), y(static_cast<int32_t>(rhs.y)) {}
inline int2::int2(const double2& rhs) : x(static_cast<int32_t>(rhs.x)), y(static_cast<int32_t>(rhs.y)) {}
inline int2::int2(const bool2& rhs) : x(static_cast<int32_t>(rhs.x)), y(static_cast<int32_t>(rhs.y)) {}
inline int2::int2(const uint2& rhs) : x(static_cast<int32_t>(rhs.x)), y(static_cast<int32_t>(rhs.y)) {}
inline int2::int2(const long2& rhs) : x(static_cast<int32_t>(rhs.x)), y(static_cast<int32_t>(rhs.y)) {}
inline int2::int2(const ulong2& rhs) : x(static_cast<int32_t>(rhs.x)), y(static_cast<int32_t>(rhs.y)) {}

// convert operator for [int3]
inline int3::int3(const float3& rhs) : x(static_cast<int32_t>(rhs.x)), y(static_cast<int32_t>(rhs.y)), z(static_cast<int32_t>(rhs.z)) {}
inline int3::int3(const double3& rhs) : x(static_cast<int32_t>(rhs.x)), y(static_cast<int32_t>(rhs.y)), z(static_cast<int32_t>(rhs.z)) {}
inline int3::int3(const bool3& rhs) : x(static_cast<int32_t>(rhs.x)), y(static_cast<int32_t>(rhs.y)), z(static_cast<int32_t>(rhs.z)) {}
inline int3::int3(const uint3& rhs) : x(static_cast<int32_t>(rhs.x)), y(static_cast<int32_t>(rhs.y)), z(static_cast<int32_t>(rhs.z)) {}
inline int3::int3(const long3& rhs) : x(static_cast<int32_t>(rhs.x)), y(static_cast<int32_t>(rhs.y)), z(static_cast<int32_t>(rhs.z)) {}
inline int3::int3(const ulong3& rhs) : x(static_cast<int32_t>(rhs.x)), y(static_cast<int32_t>(rhs.y)), z(static_cast<int32_t>(rhs.z)) {}

// convert operator for [int4]
inline int4::int4(const float4& rhs) : x(static_cast<int32_t>(rhs.x)), y(static_cast<int32_t>(rhs.y)), z(static_cast<int32_t>(rhs.z)), w(static_cast<int32_t>(rhs.w)) {}
inline int4::int4(const double4& rhs) : x(static_cast<int32_t>(rhs.x)), y(static_cast<int32_t>(rhs.y)), z(static_cast<int32_t>(rhs.z)), w(static_cast<int32_t>(rhs.w)) {}
inline int4::int4(const bool4& rhs) : x(static_cast<int32_t>(rhs.x)), y(static_cast<int32_t>(rhs.y)), z(static_cast<int32_t>(rhs.z)), w(static_cast<int32_t>(rhs.w)) {}
inline int4::int4(const uint4& rhs) : x(static_cast<int32_t>(rhs.x)), y(static_cast<int32_t>(rhs.y)), z(static_cast<int32_t>(rhs.z)), w(static_cast<int32_t>(rhs.w)) {}
inline int4::int4(const long4& rhs) : x(static_cast<int32_t>(rhs.x)), y(static_cast<int32_t>(rhs.y)), z(static_cast<int32_t>(rhs.z)), w(static_cast<int32_t>(rhs.w)) {}
inline int4::int4(const ulong4& rhs) : x(static_cast<int32_t>(rhs.x)), y(static_cast<int32_t>(rhs.y)), z(static_cast<int32_t>(rhs.z)), w(static_cast<int32_t>(rhs.w)) {}

// convert operator for [uint2]
inline uint2::uint2(const float2& rhs) : x(static_cast<uint32_t>(rhs.x)), y(static_cast<uint32_t>(rhs.y)) {}
inline uint2::uint2(const double2& rhs) : x(static_cast<uint32_t>(rhs.x)), y(static_cast<uint32_t>(rhs.y)) {}
inline uint2::uint2(const bool2& rhs) : x(static_cast<uint32_t>(rhs.x)), y(static_cast<uint32_t>(rhs.y)) {}
inline uint2::uint2(const int2& rhs) : x(static_cast<uint32_t>(rhs.x)), y(static_cast<uint32_t>(rhs.y)) {}
inline uint2::uint2(const long2& rhs) : x(static_cast<uint32_t>(rhs.x)), y(static_cast<uint32_t>(rhs.y)) {}
inline uint2::uint2(const ulong2& rhs) : x(static_cast<uint32_t>(rhs.x)), y(static_cast<uint32_t>(rhs.y)) {}

// convert operator for [uint3]
inline uint3::uint3(const float3& rhs) : x(static_cast<uint32_t>(rhs.x)), y(static_cast<uint32_t>(rhs.y)), z(static_cast<uint32_t>(rhs.z)) {}
inline uint3::uint3(const double3& rhs) : x(static_cast<uint32_t>(rhs.x)), y(static_cast<uint32_t>(rhs.y)), z(static_cast<uint32_t>(rhs.z)) {}
inline uint3::uint3(const bool3& rhs) : x(static_cast<uint32_t>(rhs.x)), y(static_cast<uint32_t>(rhs.y)), z(static_cast<uint32_t>(rhs.z)) {}
inline uint3::uint3(const int3& rhs) : x(static_cast<uint32_t>(rhs.x)), y(static_cast<uint32_t>(rhs.y)), z(static_cast<uint32_t>(rhs.z)) {}
inline uint3::uint3(const long3& rhs) : x(static_cast<uint32_t>(rhs.x)), y(static_cast<uint32_t>(rhs.y)), z(static_cast<uint32_t>(rhs.z)) {}
inline uint3::uint3(const ulong3& rhs) : x(static_cast<uint32_t>(rhs.x)), y(static_cast<uint32_t>(rhs.y)), z(static_cast<uint32_t>(rhs.z)) {}

// convert operator for [uint4]
inline uint4::uint4(const float4& rhs) : x(static_cast<uint32_t>(rhs.x)), y(static_cast<uint32_t>(rhs.y)), z(static_cast<uint32_t>(rhs.z)), w(static_cast<uint32_t>(rhs.w)) {}
inline uint4::uint4(const double4& rhs) : x(static_cast<uint32_t>(rhs.x)), y(static_cast<uint32_t>(rhs.y)), z(static_cast<uint32_t>(rhs.z)), w(static_cast<uint32_t>(rhs.w)) {}
inline uint4::uint4(const bool4& rhs) : x(static_cast<uint32_t>(rhs.x)), y(static_cast<uint32_t>(rhs.y)), z(static_cast<uint32_t>(rhs.z)), w(static_cast<uint32_t>(rhs.w)) {}
inline uint4::uint4(const int4& rhs) : x(static_cast<uint32_t>(rhs.x)), y(static_cast<uint32_t>(rhs.y)), z(static_cast<uint32_t>(rhs.z)), w(static_cast<uint32_t>(rhs.w)) {}
inline uint4::uint4(const long4& rhs) : x(static_cast<uint32_t>(rhs.x)), y(static_cast<uint32_t>(rhs.y)), z(static_cast<uint32_t>(rhs.z)), w(static_cast<uint32_t>(rhs.w)) {}
inline uint4::uint4(const ulong4& rhs) : x(static_cast<uint32_t>(rhs.x)), y(static_cast<uint32_t>(rhs.y)), z(static_cast<uint32_t>(rhs.z)), w(static_cast<uint32_t>(rhs.w)) {}

// convert operator for [long2]
inline long2::long2(const float2& rhs) : x(static_cast<int64_t>(rhs.x)), y(static_cast<int64_t>(rhs.y)) {}
inline long2::long2(const double2& rhs) : x(static_cast<int64_t>(rhs.x)), y(static_cast<int64_t>(rhs.y)) {}
inline long2::long2(const bool2& rhs) : x(static_cast<int64_t>(rhs.x)), y(static_cast<int64_t>(rhs.y)) {}
inline long2::long2(const int2& rhs) : x(static_cast<int64_t>(rhs.x)), y(static_cast<int64_t>(rhs.y)) {}
inline long2::long2(const uint2& rhs) : x(static_cast<int64_t>(rhs.x)), y(static_cast<int64_t>(rhs.y)) {}
inline long2::long2(const ulong2& rhs) : x(static_cast<int64_t>(rhs.x)), y(static_cast<int64_t>(rhs.y)) {}

// convert operator for [long3]
inline long3::long3(const float3& rhs) : x(static_cast<int64_t>(rhs.x)), y(static_cast<int64_t>(rhs.y)), z(static_cast<int64_t>(rhs.z)) {}
inline long3::long3(const double3& rhs) : x(static_cast<int64_t>(rhs.x)), y(static_cast<int64_t>(rhs.y)), z(static_cast<int64_t>(rhs.z)) {}
inline long3::long3(const bool3& rhs) : x(static_cast<int64_t>(rhs.x)), y(static_cast<int64_t>(rhs.y)), z(static_cast<int64_t>(rhs.z)) {}
inline long3::long3(const int3& rhs) : x(static_cast<int64_t>(rhs.x)), y(static_cast<int64_t>(rhs.y)), z(static_cast<int64_t>(rhs.z)) {}
inline long3::long3(const uint3& rhs) : x(static_cast<int64_t>(rhs.x)), y(static_cast<int64_t>(rhs.y)), z(static_cast<int64_t>(rhs.z)) {}
inline long3::long3(const ulong3& rhs) : x(static_cast<int64_t>(rhs.x)), y(static_cast<int64_t>(rhs.y)), z(static_cast<int64_t>(rhs.z)) {}

// convert operator for [long4]
inline long4::long4(const float4& rhs) : x(static_cast<int64_t>(rhs.x)), y(static_cast<int64_t>(rhs.y)), z(static_cast<int64_t>(rhs.z)), w(static_cast<int64_t>(rhs.w)) {}
inline long4::long4(const double4& rhs) : x(static_cast<int64_t>(rhs.x)), y(static_cast<int64_t>(rhs.y)), z(static_cast<int64_t>(rhs.z)), w(static_cast<int64_t>(rhs.w)) {}
inline long4::long4(const bool4& rhs) : x(static_cast<int64_t>(rhs.x)), y(static_cast<int64_t>(rhs.y)), z(static_cast<int64_t>(rhs.z)), w(static_cast<int64_t>(rhs.w)) {}
inline long4::long4(const int4& rhs) : x(static_cast<int64_t>(rhs.x)), y(static_cast<int64_t>(rhs.y)), z(static_cast<int64_t>(rhs.z)), w(static_cast<int64_t>(rhs.w)) {}
inline long4::long4(const uint4& rhs) : x(static_cast<int64_t>(rhs.x)), y(static_cast<int64_t>(rhs.y)), z(static_cast<int64_t>(rhs.z)), w(static_cast<int64_t>(rhs.w)) {}
inline long4::long4(const ulong4& rhs) : x(static_cast<int64_t>(rhs.x)), y(static_cast<int64_t>(rhs.y)), z(static_cast<int64_t>(rhs.z)), w(static_cast<int64_t>(rhs.w)) {}

// convert operator for [ulong2]
inline ulong2::ulong2(const float2& rhs) : x(static_cast<uint64_t>(rhs.x)), y(static_cast<uint64_t>(rhs.y)) {}
inline ulong2::ulong2(const double2& rhs) : x(static_cast<uint64_t>(rhs.x)), y(static_cast<uint64_t>(rhs.y)) {}
inline ulong2::ulong2(const bool2& rhs) : x(static_cast<uint64_t>(rhs.x)), y(static_cast<uint64_t>(rhs.y)) {}
inline ulong2::ulong2(const int2& rhs) : x(static_cast<uint64_t>(rhs.x)), y(static_cast<uint64_t>(rhs.y)) {}
inline ulong2::ulong2(const uint2& rhs) : x(static_cast<uint64_t>(rhs.x)), y(static_cast<uint64_t>(rhs.y)) {}
inline ulong2::ulong2(const long2& rhs) : x(static_cast<uint64_t>(rhs.x)), y(static_cast<uint64_t>(rhs.y)) {}

// convert operator for [ulong3]
inline ulong3::ulong3(const float3& rhs) : x(static_cast<uint64_t>(rhs.x)), y(static_cast<uint64_t>(rhs.y)), z(static_cast<uint64_t>(rhs.z)) {}
inline ulong3::ulong3(const double3& rhs) : x(static_cast<uint64_t>(rhs.x)), y(static_cast<uint64_t>(rhs.y)), z(static_cast<uint64_t>(rhs.z)) {}
inline ulong3::ulong3(const bool3& rhs) : x(static_cast<uint64_t>(rhs.x)), y(static_cast<uint64_t>(rhs.y)), z(static_cast<uint64_t>(rhs.z)) {}
inline ulong3::ulong3(const int3& rhs) : x(static_cast<uint64_t>(rhs.x)), y(static_cast<uint64_t>(rhs.y)), z(static_cast<uint64_t>(rhs.z)) {}
inline ulong3::ulong3(const uint3& rhs) : x(static_cast<uint64_t>(rhs.x)), y(static_cast<uint64_t>(rhs.y)), z(static_cast<uint64_t>(rhs.z)) {}
inline ulong3::ulong3(const long3& rhs) : x(static_cast<uint64_t>(rhs.x)), y(static_cast<uint64_t>(rhs.y)), z(static_cast<uint64_t>(rhs.z)) {}

// convert operator for [ulong4]
inline ulong4::ulong4(const float4& rhs) : x(static_cast<uint64_t>(rhs.x)), y(static_cast<uint64_t>(rhs.y)), z(static_cast<uint64_t>(rhs.z)), w(static_cast<uint64_t>(rhs.w)) {}
inline ulong4::ulong4(const double4& rhs) : x(static_cast<uint64_t>(rhs.x)), y(static_cast<uint64_t>(rhs.y)), z(static_cast<uint64_t>(rhs.z)), w(static_cast<uint64_t>(rhs.w)) {}
inline ulong4::ulong4(const bool4& rhs) : x(static_cast<uint64_t>(rhs.x)), y(static_cast<uint64_t>(rhs.y)), z(static_cast<uint64_t>(rhs.z)), w(static_cast<uint64_t>(rhs.w)) {}
inline ulong4::ulong4(const int4& rhs) : x(static_cast<uint64_t>(rhs.x)), y(static_cast<uint64_t>(rhs.y)), z(static_cast<uint64_t>(rhs.z)), w(static_cast<uint64_t>(rhs.w)) {}
inline ulong4::ulong4(const uint4& rhs) : x(static_cast<uint64_t>(rhs.x)), y(static_cast<uint64_t>(rhs.y)), z(static_cast<uint64_t>(rhs.z)), w(static_cast<uint64_t>(rhs.w)) {}
inline ulong4::ulong4(const long4& rhs) : x(static_cast<uint64_t>(rhs.x)), y(static_cast<uint64_t>(rhs.y)), z(static_cast<uint64_t>(rhs.z)), w(static_cast<uint64_t>(rhs.w)) {}

}
}
