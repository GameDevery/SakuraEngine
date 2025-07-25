//! *************************************************************************
//! **  This file is auto-generated by gen_math, do not edit it manually.  **
//! *************************************************************************

#pragma once
#include <cstdint>
#include <cmath>
#include "../gen_math_fwd.hpp"
#include "../../math_constants.hpp"
#include <SkrBase/misc/debug.h>
#include <SkrBase/misc/hash.hpp>

namespace skr {
inline namespace math {
struct int2 {
    int32_t x, y;
    
    // ctor & dtor
    inline int2(): x(0), y(0) {}
    inline int2(MathNoInitType) {}
    inline int2(int32_t v): x(v), y(v) {}
    inline int2(int32_t v_x, int32_t v_y): x(v_x), y(v_y) {}
    inline ~int2() = default;
    
    // cast ctor
    explicit int2(const float2& rhs);
    explicit int2(const double2& rhs);
    int2(const bool2& rhs);
    explicit int2(const uint2& rhs);
    explicit int2(const long2& rhs);
    explicit int2(const ulong2& rhs);
    
    // copy & move & assign & move assign
    inline int2(const int2&) = default;
    inline int2(int2&&) = default;
    inline int2& operator=(const int2&) = default;
    inline int2& operator=(int2&&) = default;
    
    // array assessor
    inline int32_t& operator[](size_t i) {
        SKR_ASSERT(i >= 0 && i < 2 && "index out of range");
        return reinterpret_cast<int32_t*>(this)[i];
    }
    inline int32_t operator[](size_t i) const {
        return const_cast<int2*>(this)->operator[](i);
    }
    
    // unary operator
    inline int2 operator-() const { return { -x, -y }; }
    
    // arithmetic operator
    inline friend int2 operator+(const int2& lhs, const int2& rhs) { return { lhs.x + rhs.x, lhs.y + rhs.y }; }
    inline friend int2 operator-(const int2& lhs, const int2& rhs) { return { lhs.x - rhs.x, lhs.y - rhs.y }; }
    inline friend int2 operator*(const int2& lhs, const int2& rhs) { return { lhs.x * rhs.x, lhs.y * rhs.y }; }
    inline friend int2 operator/(const int2& lhs, const int2& rhs) { return { lhs.x / rhs.x, lhs.y / rhs.y }; }
    inline friend int2 operator%(const int2& lhs, const int2& rhs) { return { lhs.x % rhs.x, lhs.y % rhs.y }; }
    
    // arithmetic assign operator
    inline int2& operator+=(const int2& rhs) { x += rhs.x, y += rhs.y; return *this; }
    inline int2& operator-=(const int2& rhs) { x -= rhs.x, y -= rhs.y; return *this; }
    inline int2& operator*=(const int2& rhs) { x *= rhs.x, y *= rhs.y; return *this; }
    inline int2& operator/=(const int2& rhs) { x /= rhs.x, y /= rhs.y; return *this; }
    inline int2& operator%=(const int2& rhs) { x %= rhs.x, y %= rhs.y; return *this; }
    
    // compare operator
    friend bool2 operator==(const int2& lhs, const int2& rhs);
    friend bool2 operator!=(const int2& lhs, const int2& rhs);
    friend bool2 operator<(const int2& lhs, const int2& rhs);
    friend bool2 operator<=(const int2& lhs, const int2& rhs);
    friend bool2 operator>(const int2& lhs, const int2& rhs);
    friend bool2 operator>=(const int2& lhs, const int2& rhs);
    
    // swizzle
    inline int2 xx() const { return {x, x}; }
    inline int2 xy() const { return {x, y}; }
    inline void set_xy(const int2& v) { x = v.x; y = v.y; }
    inline int2 yx() const { return {y, x}; }
    inline void set_yx(const int2& v) { y = v.x; x = v.y; }
    inline int2 yy() const { return {y, y}; }
    
    // hash
    inline static size_t _skr_hash(const int2& v) {
        auto hasher = ::skr::Hash<int32_t>{};
        auto result = hasher(v.x);
        result = ::skr::hash_combine(result, hasher(v.y));
        return result;
    }
};
struct int3 {
    int32_t x, y, z;
    
    // ctor & dtor
    inline int3(): x(0), y(0), z(0) {}
    inline int3(MathNoInitType) {}
    inline int3(int32_t v): x(v), y(v), z(v) {}
    inline int3(int32_t v_x, int32_t v_y, int32_t v_z): x(v_x), y(v_y), z(v_z) {}
    inline int3(int32_t v_x, int2 v_yz): x(v_x), y(v_yz.x), z(v_yz.y) {}
    inline int3(int2 v_xy, int32_t v_z): x(v_xy.x), y(v_xy.y), z(v_z) {}
    inline ~int3() = default;
    
    // cast ctor
    explicit int3(const float3& rhs);
    explicit int3(const double3& rhs);
    int3(const bool3& rhs);
    explicit int3(const uint3& rhs);
    explicit int3(const long3& rhs);
    explicit int3(const ulong3& rhs);
    
    // copy & move & assign & move assign
    inline int3(const int3&) = default;
    inline int3(int3&&) = default;
    inline int3& operator=(const int3&) = default;
    inline int3& operator=(int3&&) = default;
    
    // array assessor
    inline int32_t& operator[](size_t i) {
        SKR_ASSERT(i >= 0 && i < 3 && "index out of range");
        return reinterpret_cast<int32_t*>(this)[i];
    }
    inline int32_t operator[](size_t i) const {
        return const_cast<int3*>(this)->operator[](i);
    }
    
    // unary operator
    inline int3 operator-() const { return { -x, -y, -z }; }
    
    // arithmetic operator
    inline friend int3 operator+(const int3& lhs, const int3& rhs) { return { lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z }; }
    inline friend int3 operator-(const int3& lhs, const int3& rhs) { return { lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z }; }
    inline friend int3 operator*(const int3& lhs, const int3& rhs) { return { lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z }; }
    inline friend int3 operator/(const int3& lhs, const int3& rhs) { return { lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z }; }
    inline friend int3 operator%(const int3& lhs, const int3& rhs) { return { lhs.x % rhs.x, lhs.y % rhs.y, lhs.z % rhs.z }; }
    
    // arithmetic assign operator
    inline int3& operator+=(const int3& rhs) { x += rhs.x, y += rhs.y, z += rhs.z; return *this; }
    inline int3& operator-=(const int3& rhs) { x -= rhs.x, y -= rhs.y, z -= rhs.z; return *this; }
    inline int3& operator*=(const int3& rhs) { x *= rhs.x, y *= rhs.y, z *= rhs.z; return *this; }
    inline int3& operator/=(const int3& rhs) { x /= rhs.x, y /= rhs.y, z /= rhs.z; return *this; }
    inline int3& operator%=(const int3& rhs) { x %= rhs.x, y %= rhs.y, z %= rhs.z; return *this; }
    
    // compare operator
    friend bool3 operator==(const int3& lhs, const int3& rhs);
    friend bool3 operator!=(const int3& lhs, const int3& rhs);
    friend bool3 operator<(const int3& lhs, const int3& rhs);
    friend bool3 operator<=(const int3& lhs, const int3& rhs);
    friend bool3 operator>(const int3& lhs, const int3& rhs);
    friend bool3 operator>=(const int3& lhs, const int3& rhs);
    
    // swizzle
    inline int2 xx() const { return {x, x}; }
    inline int2 xy() const { return {x, y}; }
    inline void set_xy(const int2& v) { x = v.x; y = v.y; }
    inline int2 xz() const { return {x, z}; }
    inline void set_xz(const int2& v) { x = v.x; z = v.y; }
    inline int2 yx() const { return {y, x}; }
    inline void set_yx(const int2& v) { y = v.x; x = v.y; }
    inline int2 yy() const { return {y, y}; }
    inline int2 yz() const { return {y, z}; }
    inline void set_yz(const int2& v) { y = v.x; z = v.y; }
    inline int2 zx() const { return {z, x}; }
    inline void set_zx(const int2& v) { z = v.x; x = v.y; }
    inline int2 zy() const { return {z, y}; }
    inline void set_zy(const int2& v) { z = v.x; y = v.y; }
    inline int2 zz() const { return {z, z}; }
    inline int3 xxx() const { return {x, x, x}; }
    inline int3 xxy() const { return {x, x, y}; }
    inline int3 xxz() const { return {x, x, z}; }
    inline int3 xyx() const { return {x, y, x}; }
    inline int3 xyy() const { return {x, y, y}; }
    inline int3 xyz() const { return {x, y, z}; }
    inline void set_xyz(const int3& v) { x = v.x; y = v.y; z = v.z; }
    inline int3 xzx() const { return {x, z, x}; }
    inline int3 xzy() const { return {x, z, y}; }
    inline void set_xzy(const int3& v) { x = v.x; z = v.y; y = v.z; }
    inline int3 xzz() const { return {x, z, z}; }
    inline int3 yxx() const { return {y, x, x}; }
    inline int3 yxy() const { return {y, x, y}; }
    inline int3 yxz() const { return {y, x, z}; }
    inline void set_yxz(const int3& v) { y = v.x; x = v.y; z = v.z; }
    inline int3 yyx() const { return {y, y, x}; }
    inline int3 yyy() const { return {y, y, y}; }
    inline int3 yyz() const { return {y, y, z}; }
    inline int3 yzx() const { return {y, z, x}; }
    inline void set_yzx(const int3& v) { y = v.x; z = v.y; x = v.z; }
    inline int3 yzy() const { return {y, z, y}; }
    inline int3 yzz() const { return {y, z, z}; }
    inline int3 zxx() const { return {z, x, x}; }
    inline int3 zxy() const { return {z, x, y}; }
    inline void set_zxy(const int3& v) { z = v.x; x = v.y; y = v.z; }
    inline int3 zxz() const { return {z, x, z}; }
    inline int3 zyx() const { return {z, y, x}; }
    inline void set_zyx(const int3& v) { z = v.x; y = v.y; x = v.z; }
    inline int3 zyy() const { return {z, y, y}; }
    inline int3 zyz() const { return {z, y, z}; }
    inline int3 zzx() const { return {z, z, x}; }
    inline int3 zzy() const { return {z, z, y}; }
    inline int3 zzz() const { return {z, z, z}; }
    
    // hash
    inline static size_t _skr_hash(const int3& v) {
        auto hasher = ::skr::Hash<int32_t>{};
        auto result = hasher(v.x);
        result = ::skr::hash_combine(result, hasher(v.y));
        result = ::skr::hash_combine(result, hasher(v.z));
        return result;
    }
};
struct int4 {
    int32_t x, y, z, w;
    
    // ctor & dtor
    inline int4(): x(0), y(0), z(0), w(0) {}
    inline int4(MathNoInitType) {}
    inline int4(int32_t v): x(v), y(v), z(v), w(v) {}
    inline int4(int32_t v_x, int32_t v_y, int32_t v_z, int32_t v_w): x(v_x), y(v_y), z(v_z), w(v_w) {}
    inline int4(int32_t v_x, int32_t v_y, int2 v_zw): x(v_x), y(v_y), z(v_zw.x), w(v_zw.y) {}
    inline int4(int32_t v_x, int2 v_yz, int32_t v_w): x(v_x), y(v_yz.x), z(v_yz.y), w(v_w) {}
    inline int4(int32_t v_x, int3 v_yzw): x(v_x), y(v_yzw.x), z(v_yzw.y), w(v_yzw.z) {}
    inline int4(int2 v_xy, int32_t v_z, int32_t v_w): x(v_xy.x), y(v_xy.y), z(v_z), w(v_w) {}
    inline int4(int2 v_xy, int2 v_zw): x(v_xy.x), y(v_xy.y), z(v_zw.x), w(v_zw.y) {}
    inline int4(int3 v_xyz, int32_t v_w): x(v_xyz.x), y(v_xyz.y), z(v_xyz.z), w(v_w) {}
    inline ~int4() = default;
    
    // cast ctor
    explicit int4(const float4& rhs);
    explicit int4(const double4& rhs);
    int4(const bool4& rhs);
    explicit int4(const uint4& rhs);
    explicit int4(const long4& rhs);
    explicit int4(const ulong4& rhs);
    
    // copy & move & assign & move assign
    inline int4(const int4&) = default;
    inline int4(int4&&) = default;
    inline int4& operator=(const int4&) = default;
    inline int4& operator=(int4&&) = default;
    
    // array assessor
    inline int32_t& operator[](size_t i) {
        SKR_ASSERT(i >= 0 && i < 4 && "index out of range");
        return reinterpret_cast<int32_t*>(this)[i];
    }
    inline int32_t operator[](size_t i) const {
        return const_cast<int4*>(this)->operator[](i);
    }
    
    // unary operator
    inline int4 operator-() const { return { -x, -y, -z, -w }; }
    
    // arithmetic operator
    inline friend int4 operator+(const int4& lhs, const int4& rhs) { return { lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w }; }
    inline friend int4 operator-(const int4& lhs, const int4& rhs) { return { lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w }; }
    inline friend int4 operator*(const int4& lhs, const int4& rhs) { return { lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w }; }
    inline friend int4 operator/(const int4& lhs, const int4& rhs) { return { lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z, lhs.w / rhs.w }; }
    inline friend int4 operator%(const int4& lhs, const int4& rhs) { return { lhs.x % rhs.x, lhs.y % rhs.y, lhs.z % rhs.z, lhs.w % rhs.w }; }
    
    // arithmetic assign operator
    inline int4& operator+=(const int4& rhs) { x += rhs.x, y += rhs.y, z += rhs.z, w += rhs.w; return *this; }
    inline int4& operator-=(const int4& rhs) { x -= rhs.x, y -= rhs.y, z -= rhs.z, w -= rhs.w; return *this; }
    inline int4& operator*=(const int4& rhs) { x *= rhs.x, y *= rhs.y, z *= rhs.z, w *= rhs.w; return *this; }
    inline int4& operator/=(const int4& rhs) { x /= rhs.x, y /= rhs.y, z /= rhs.z, w /= rhs.w; return *this; }
    inline int4& operator%=(const int4& rhs) { x %= rhs.x, y %= rhs.y, z %= rhs.z, w %= rhs.w; return *this; }
    
    // compare operator
    friend bool4 operator==(const int4& lhs, const int4& rhs);
    friend bool4 operator!=(const int4& lhs, const int4& rhs);
    friend bool4 operator<(const int4& lhs, const int4& rhs);
    friend bool4 operator<=(const int4& lhs, const int4& rhs);
    friend bool4 operator>(const int4& lhs, const int4& rhs);
    friend bool4 operator>=(const int4& lhs, const int4& rhs);
    
    // swizzle
    inline int2 xx() const { return {x, x}; }
    inline int2 xy() const { return {x, y}; }
    inline void set_xy(const int2& v) { x = v.x; y = v.y; }
    inline int2 xz() const { return {x, z}; }
    inline void set_xz(const int2& v) { x = v.x; z = v.y; }
    inline int2 xw() const { return {x, w}; }
    inline void set_xw(const int2& v) { x = v.x; w = v.y; }
    inline int2 yx() const { return {y, x}; }
    inline void set_yx(const int2& v) { y = v.x; x = v.y; }
    inline int2 yy() const { return {y, y}; }
    inline int2 yz() const { return {y, z}; }
    inline void set_yz(const int2& v) { y = v.x; z = v.y; }
    inline int2 yw() const { return {y, w}; }
    inline void set_yw(const int2& v) { y = v.x; w = v.y; }
    inline int2 zx() const { return {z, x}; }
    inline void set_zx(const int2& v) { z = v.x; x = v.y; }
    inline int2 zy() const { return {z, y}; }
    inline void set_zy(const int2& v) { z = v.x; y = v.y; }
    inline int2 zz() const { return {z, z}; }
    inline int2 zw() const { return {z, w}; }
    inline void set_zw(const int2& v) { z = v.x; w = v.y; }
    inline int2 wx() const { return {w, x}; }
    inline void set_wx(const int2& v) { w = v.x; x = v.y; }
    inline int2 wy() const { return {w, y}; }
    inline void set_wy(const int2& v) { w = v.x; y = v.y; }
    inline int2 wz() const { return {w, z}; }
    inline void set_wz(const int2& v) { w = v.x; z = v.y; }
    inline int2 ww() const { return {w, w}; }
    inline int3 xxx() const { return {x, x, x}; }
    inline int3 xxy() const { return {x, x, y}; }
    inline int3 xxz() const { return {x, x, z}; }
    inline int3 xxw() const { return {x, x, w}; }
    inline int3 xyx() const { return {x, y, x}; }
    inline int3 xyy() const { return {x, y, y}; }
    inline int3 xyz() const { return {x, y, z}; }
    inline void set_xyz(const int3& v) { x = v.x; y = v.y; z = v.z; }
    inline int3 xyw() const { return {x, y, w}; }
    inline void set_xyw(const int3& v) { x = v.x; y = v.y; w = v.z; }
    inline int3 xzx() const { return {x, z, x}; }
    inline int3 xzy() const { return {x, z, y}; }
    inline void set_xzy(const int3& v) { x = v.x; z = v.y; y = v.z; }
    inline int3 xzz() const { return {x, z, z}; }
    inline int3 xzw() const { return {x, z, w}; }
    inline void set_xzw(const int3& v) { x = v.x; z = v.y; w = v.z; }
    inline int3 xwx() const { return {x, w, x}; }
    inline int3 xwy() const { return {x, w, y}; }
    inline void set_xwy(const int3& v) { x = v.x; w = v.y; y = v.z; }
    inline int3 xwz() const { return {x, w, z}; }
    inline void set_xwz(const int3& v) { x = v.x; w = v.y; z = v.z; }
    inline int3 xww() const { return {x, w, w}; }
    inline int3 yxx() const { return {y, x, x}; }
    inline int3 yxy() const { return {y, x, y}; }
    inline int3 yxz() const { return {y, x, z}; }
    inline void set_yxz(const int3& v) { y = v.x; x = v.y; z = v.z; }
    inline int3 yxw() const { return {y, x, w}; }
    inline void set_yxw(const int3& v) { y = v.x; x = v.y; w = v.z; }
    inline int3 yyx() const { return {y, y, x}; }
    inline int3 yyy() const { return {y, y, y}; }
    inline int3 yyz() const { return {y, y, z}; }
    inline int3 yyw() const { return {y, y, w}; }
    inline int3 yzx() const { return {y, z, x}; }
    inline void set_yzx(const int3& v) { y = v.x; z = v.y; x = v.z; }
    inline int3 yzy() const { return {y, z, y}; }
    inline int3 yzz() const { return {y, z, z}; }
    inline int3 yzw() const { return {y, z, w}; }
    inline void set_yzw(const int3& v) { y = v.x; z = v.y; w = v.z; }
    inline int3 ywx() const { return {y, w, x}; }
    inline void set_ywx(const int3& v) { y = v.x; w = v.y; x = v.z; }
    inline int3 ywy() const { return {y, w, y}; }
    inline int3 ywz() const { return {y, w, z}; }
    inline void set_ywz(const int3& v) { y = v.x; w = v.y; z = v.z; }
    inline int3 yww() const { return {y, w, w}; }
    inline int3 zxx() const { return {z, x, x}; }
    inline int3 zxy() const { return {z, x, y}; }
    inline void set_zxy(const int3& v) { z = v.x; x = v.y; y = v.z; }
    inline int3 zxz() const { return {z, x, z}; }
    inline int3 zxw() const { return {z, x, w}; }
    inline void set_zxw(const int3& v) { z = v.x; x = v.y; w = v.z; }
    inline int3 zyx() const { return {z, y, x}; }
    inline void set_zyx(const int3& v) { z = v.x; y = v.y; x = v.z; }
    inline int3 zyy() const { return {z, y, y}; }
    inline int3 zyz() const { return {z, y, z}; }
    inline int3 zyw() const { return {z, y, w}; }
    inline void set_zyw(const int3& v) { z = v.x; y = v.y; w = v.z; }
    inline int3 zzx() const { return {z, z, x}; }
    inline int3 zzy() const { return {z, z, y}; }
    inline int3 zzz() const { return {z, z, z}; }
    inline int3 zzw() const { return {z, z, w}; }
    inline int3 zwx() const { return {z, w, x}; }
    inline void set_zwx(const int3& v) { z = v.x; w = v.y; x = v.z; }
    inline int3 zwy() const { return {z, w, y}; }
    inline void set_zwy(const int3& v) { z = v.x; w = v.y; y = v.z; }
    inline int3 zwz() const { return {z, w, z}; }
    inline int3 zww() const { return {z, w, w}; }
    inline int3 wxx() const { return {w, x, x}; }
    inline int3 wxy() const { return {w, x, y}; }
    inline void set_wxy(const int3& v) { w = v.x; x = v.y; y = v.z; }
    inline int3 wxz() const { return {w, x, z}; }
    inline void set_wxz(const int3& v) { w = v.x; x = v.y; z = v.z; }
    inline int3 wxw() const { return {w, x, w}; }
    inline int3 wyx() const { return {w, y, x}; }
    inline void set_wyx(const int3& v) { w = v.x; y = v.y; x = v.z; }
    inline int3 wyy() const { return {w, y, y}; }
    inline int3 wyz() const { return {w, y, z}; }
    inline void set_wyz(const int3& v) { w = v.x; y = v.y; z = v.z; }
    inline int3 wyw() const { return {w, y, w}; }
    inline int3 wzx() const { return {w, z, x}; }
    inline void set_wzx(const int3& v) { w = v.x; z = v.y; x = v.z; }
    inline int3 wzy() const { return {w, z, y}; }
    inline void set_wzy(const int3& v) { w = v.x; z = v.y; y = v.z; }
    inline int3 wzz() const { return {w, z, z}; }
    inline int3 wzw() const { return {w, z, w}; }
    inline int3 wwx() const { return {w, w, x}; }
    inline int3 wwy() const { return {w, w, y}; }
    inline int3 wwz() const { return {w, w, z}; }
    inline int3 www() const { return {w, w, w}; }
    inline int4 xxxx() const { return {x, x, x, x}; }
    inline int4 xxxy() const { return {x, x, x, y}; }
    inline int4 xxxz() const { return {x, x, x, z}; }
    inline int4 xxxw() const { return {x, x, x, w}; }
    inline int4 xxyx() const { return {x, x, y, x}; }
    inline int4 xxyy() const { return {x, x, y, y}; }
    inline int4 xxyz() const { return {x, x, y, z}; }
    inline int4 xxyw() const { return {x, x, y, w}; }
    inline int4 xxzx() const { return {x, x, z, x}; }
    inline int4 xxzy() const { return {x, x, z, y}; }
    inline int4 xxzz() const { return {x, x, z, z}; }
    inline int4 xxzw() const { return {x, x, z, w}; }
    inline int4 xxwx() const { return {x, x, w, x}; }
    inline int4 xxwy() const { return {x, x, w, y}; }
    inline int4 xxwz() const { return {x, x, w, z}; }
    inline int4 xxww() const { return {x, x, w, w}; }
    inline int4 xyxx() const { return {x, y, x, x}; }
    inline int4 xyxy() const { return {x, y, x, y}; }
    inline int4 xyxz() const { return {x, y, x, z}; }
    inline int4 xyxw() const { return {x, y, x, w}; }
    inline int4 xyyx() const { return {x, y, y, x}; }
    inline int4 xyyy() const { return {x, y, y, y}; }
    inline int4 xyyz() const { return {x, y, y, z}; }
    inline int4 xyyw() const { return {x, y, y, w}; }
    inline int4 xyzx() const { return {x, y, z, x}; }
    inline int4 xyzy() const { return {x, y, z, y}; }
    inline int4 xyzz() const { return {x, y, z, z}; }
    inline int4 xyzw() const { return {x, y, z, w}; }
    inline void set_xyzw(const int4& v) { x = v.x; y = v.y; z = v.z; w = v.w; }
    inline int4 xywx() const { return {x, y, w, x}; }
    inline int4 xywy() const { return {x, y, w, y}; }
    inline int4 xywz() const { return {x, y, w, z}; }
    inline void set_xywz(const int4& v) { x = v.x; y = v.y; w = v.z; z = v.w; }
    inline int4 xyww() const { return {x, y, w, w}; }
    inline int4 xzxx() const { return {x, z, x, x}; }
    inline int4 xzxy() const { return {x, z, x, y}; }
    inline int4 xzxz() const { return {x, z, x, z}; }
    inline int4 xzxw() const { return {x, z, x, w}; }
    inline int4 xzyx() const { return {x, z, y, x}; }
    inline int4 xzyy() const { return {x, z, y, y}; }
    inline int4 xzyz() const { return {x, z, y, z}; }
    inline int4 xzyw() const { return {x, z, y, w}; }
    inline void set_xzyw(const int4& v) { x = v.x; z = v.y; y = v.z; w = v.w; }
    inline int4 xzzx() const { return {x, z, z, x}; }
    inline int4 xzzy() const { return {x, z, z, y}; }
    inline int4 xzzz() const { return {x, z, z, z}; }
    inline int4 xzzw() const { return {x, z, z, w}; }
    inline int4 xzwx() const { return {x, z, w, x}; }
    inline int4 xzwy() const { return {x, z, w, y}; }
    inline void set_xzwy(const int4& v) { x = v.x; z = v.y; w = v.z; y = v.w; }
    inline int4 xzwz() const { return {x, z, w, z}; }
    inline int4 xzww() const { return {x, z, w, w}; }
    inline int4 xwxx() const { return {x, w, x, x}; }
    inline int4 xwxy() const { return {x, w, x, y}; }
    inline int4 xwxz() const { return {x, w, x, z}; }
    inline int4 xwxw() const { return {x, w, x, w}; }
    inline int4 xwyx() const { return {x, w, y, x}; }
    inline int4 xwyy() const { return {x, w, y, y}; }
    inline int4 xwyz() const { return {x, w, y, z}; }
    inline void set_xwyz(const int4& v) { x = v.x; w = v.y; y = v.z; z = v.w; }
    inline int4 xwyw() const { return {x, w, y, w}; }
    inline int4 xwzx() const { return {x, w, z, x}; }
    inline int4 xwzy() const { return {x, w, z, y}; }
    inline void set_xwzy(const int4& v) { x = v.x; w = v.y; z = v.z; y = v.w; }
    inline int4 xwzz() const { return {x, w, z, z}; }
    inline int4 xwzw() const { return {x, w, z, w}; }
    inline int4 xwwx() const { return {x, w, w, x}; }
    inline int4 xwwy() const { return {x, w, w, y}; }
    inline int4 xwwz() const { return {x, w, w, z}; }
    inline int4 xwww() const { return {x, w, w, w}; }
    inline int4 yxxx() const { return {y, x, x, x}; }
    inline int4 yxxy() const { return {y, x, x, y}; }
    inline int4 yxxz() const { return {y, x, x, z}; }
    inline int4 yxxw() const { return {y, x, x, w}; }
    inline int4 yxyx() const { return {y, x, y, x}; }
    inline int4 yxyy() const { return {y, x, y, y}; }
    inline int4 yxyz() const { return {y, x, y, z}; }
    inline int4 yxyw() const { return {y, x, y, w}; }
    inline int4 yxzx() const { return {y, x, z, x}; }
    inline int4 yxzy() const { return {y, x, z, y}; }
    inline int4 yxzz() const { return {y, x, z, z}; }
    inline int4 yxzw() const { return {y, x, z, w}; }
    inline void set_yxzw(const int4& v) { y = v.x; x = v.y; z = v.z; w = v.w; }
    inline int4 yxwx() const { return {y, x, w, x}; }
    inline int4 yxwy() const { return {y, x, w, y}; }
    inline int4 yxwz() const { return {y, x, w, z}; }
    inline void set_yxwz(const int4& v) { y = v.x; x = v.y; w = v.z; z = v.w; }
    inline int4 yxww() const { return {y, x, w, w}; }
    inline int4 yyxx() const { return {y, y, x, x}; }
    inline int4 yyxy() const { return {y, y, x, y}; }
    inline int4 yyxz() const { return {y, y, x, z}; }
    inline int4 yyxw() const { return {y, y, x, w}; }
    inline int4 yyyx() const { return {y, y, y, x}; }
    inline int4 yyyy() const { return {y, y, y, y}; }
    inline int4 yyyz() const { return {y, y, y, z}; }
    inline int4 yyyw() const { return {y, y, y, w}; }
    inline int4 yyzx() const { return {y, y, z, x}; }
    inline int4 yyzy() const { return {y, y, z, y}; }
    inline int4 yyzz() const { return {y, y, z, z}; }
    inline int4 yyzw() const { return {y, y, z, w}; }
    inline int4 yywx() const { return {y, y, w, x}; }
    inline int4 yywy() const { return {y, y, w, y}; }
    inline int4 yywz() const { return {y, y, w, z}; }
    inline int4 yyww() const { return {y, y, w, w}; }
    inline int4 yzxx() const { return {y, z, x, x}; }
    inline int4 yzxy() const { return {y, z, x, y}; }
    inline int4 yzxz() const { return {y, z, x, z}; }
    inline int4 yzxw() const { return {y, z, x, w}; }
    inline void set_yzxw(const int4& v) { y = v.x; z = v.y; x = v.z; w = v.w; }
    inline int4 yzyx() const { return {y, z, y, x}; }
    inline int4 yzyy() const { return {y, z, y, y}; }
    inline int4 yzyz() const { return {y, z, y, z}; }
    inline int4 yzyw() const { return {y, z, y, w}; }
    inline int4 yzzx() const { return {y, z, z, x}; }
    inline int4 yzzy() const { return {y, z, z, y}; }
    inline int4 yzzz() const { return {y, z, z, z}; }
    inline int4 yzzw() const { return {y, z, z, w}; }
    inline int4 yzwx() const { return {y, z, w, x}; }
    inline void set_yzwx(const int4& v) { y = v.x; z = v.y; w = v.z; x = v.w; }
    inline int4 yzwy() const { return {y, z, w, y}; }
    inline int4 yzwz() const { return {y, z, w, z}; }
    inline int4 yzww() const { return {y, z, w, w}; }
    inline int4 ywxx() const { return {y, w, x, x}; }
    inline int4 ywxy() const { return {y, w, x, y}; }
    inline int4 ywxz() const { return {y, w, x, z}; }
    inline void set_ywxz(const int4& v) { y = v.x; w = v.y; x = v.z; z = v.w; }
    inline int4 ywxw() const { return {y, w, x, w}; }
    inline int4 ywyx() const { return {y, w, y, x}; }
    inline int4 ywyy() const { return {y, w, y, y}; }
    inline int4 ywyz() const { return {y, w, y, z}; }
    inline int4 ywyw() const { return {y, w, y, w}; }
    inline int4 ywzx() const { return {y, w, z, x}; }
    inline void set_ywzx(const int4& v) { y = v.x; w = v.y; z = v.z; x = v.w; }
    inline int4 ywzy() const { return {y, w, z, y}; }
    inline int4 ywzz() const { return {y, w, z, z}; }
    inline int4 ywzw() const { return {y, w, z, w}; }
    inline int4 ywwx() const { return {y, w, w, x}; }
    inline int4 ywwy() const { return {y, w, w, y}; }
    inline int4 ywwz() const { return {y, w, w, z}; }
    inline int4 ywww() const { return {y, w, w, w}; }
    inline int4 zxxx() const { return {z, x, x, x}; }
    inline int4 zxxy() const { return {z, x, x, y}; }
    inline int4 zxxz() const { return {z, x, x, z}; }
    inline int4 zxxw() const { return {z, x, x, w}; }
    inline int4 zxyx() const { return {z, x, y, x}; }
    inline int4 zxyy() const { return {z, x, y, y}; }
    inline int4 zxyz() const { return {z, x, y, z}; }
    inline int4 zxyw() const { return {z, x, y, w}; }
    inline void set_zxyw(const int4& v) { z = v.x; x = v.y; y = v.z; w = v.w; }
    inline int4 zxzx() const { return {z, x, z, x}; }
    inline int4 zxzy() const { return {z, x, z, y}; }
    inline int4 zxzz() const { return {z, x, z, z}; }
    inline int4 zxzw() const { return {z, x, z, w}; }
    inline int4 zxwx() const { return {z, x, w, x}; }
    inline int4 zxwy() const { return {z, x, w, y}; }
    inline void set_zxwy(const int4& v) { z = v.x; x = v.y; w = v.z; y = v.w; }
    inline int4 zxwz() const { return {z, x, w, z}; }
    inline int4 zxww() const { return {z, x, w, w}; }
    inline int4 zyxx() const { return {z, y, x, x}; }
    inline int4 zyxy() const { return {z, y, x, y}; }
    inline int4 zyxz() const { return {z, y, x, z}; }
    inline int4 zyxw() const { return {z, y, x, w}; }
    inline void set_zyxw(const int4& v) { z = v.x; y = v.y; x = v.z; w = v.w; }
    inline int4 zyyx() const { return {z, y, y, x}; }
    inline int4 zyyy() const { return {z, y, y, y}; }
    inline int4 zyyz() const { return {z, y, y, z}; }
    inline int4 zyyw() const { return {z, y, y, w}; }
    inline int4 zyzx() const { return {z, y, z, x}; }
    inline int4 zyzy() const { return {z, y, z, y}; }
    inline int4 zyzz() const { return {z, y, z, z}; }
    inline int4 zyzw() const { return {z, y, z, w}; }
    inline int4 zywx() const { return {z, y, w, x}; }
    inline void set_zywx(const int4& v) { z = v.x; y = v.y; w = v.z; x = v.w; }
    inline int4 zywy() const { return {z, y, w, y}; }
    inline int4 zywz() const { return {z, y, w, z}; }
    inline int4 zyww() const { return {z, y, w, w}; }
    inline int4 zzxx() const { return {z, z, x, x}; }
    inline int4 zzxy() const { return {z, z, x, y}; }
    inline int4 zzxz() const { return {z, z, x, z}; }
    inline int4 zzxw() const { return {z, z, x, w}; }
    inline int4 zzyx() const { return {z, z, y, x}; }
    inline int4 zzyy() const { return {z, z, y, y}; }
    inline int4 zzyz() const { return {z, z, y, z}; }
    inline int4 zzyw() const { return {z, z, y, w}; }
    inline int4 zzzx() const { return {z, z, z, x}; }
    inline int4 zzzy() const { return {z, z, z, y}; }
    inline int4 zzzz() const { return {z, z, z, z}; }
    inline int4 zzzw() const { return {z, z, z, w}; }
    inline int4 zzwx() const { return {z, z, w, x}; }
    inline int4 zzwy() const { return {z, z, w, y}; }
    inline int4 zzwz() const { return {z, z, w, z}; }
    inline int4 zzww() const { return {z, z, w, w}; }
    inline int4 zwxx() const { return {z, w, x, x}; }
    inline int4 zwxy() const { return {z, w, x, y}; }
    inline void set_zwxy(const int4& v) { z = v.x; w = v.y; x = v.z; y = v.w; }
    inline int4 zwxz() const { return {z, w, x, z}; }
    inline int4 zwxw() const { return {z, w, x, w}; }
    inline int4 zwyx() const { return {z, w, y, x}; }
    inline void set_zwyx(const int4& v) { z = v.x; w = v.y; y = v.z; x = v.w; }
    inline int4 zwyy() const { return {z, w, y, y}; }
    inline int4 zwyz() const { return {z, w, y, z}; }
    inline int4 zwyw() const { return {z, w, y, w}; }
    inline int4 zwzx() const { return {z, w, z, x}; }
    inline int4 zwzy() const { return {z, w, z, y}; }
    inline int4 zwzz() const { return {z, w, z, z}; }
    inline int4 zwzw() const { return {z, w, z, w}; }
    inline int4 zwwx() const { return {z, w, w, x}; }
    inline int4 zwwy() const { return {z, w, w, y}; }
    inline int4 zwwz() const { return {z, w, w, z}; }
    inline int4 zwww() const { return {z, w, w, w}; }
    inline int4 wxxx() const { return {w, x, x, x}; }
    inline int4 wxxy() const { return {w, x, x, y}; }
    inline int4 wxxz() const { return {w, x, x, z}; }
    inline int4 wxxw() const { return {w, x, x, w}; }
    inline int4 wxyx() const { return {w, x, y, x}; }
    inline int4 wxyy() const { return {w, x, y, y}; }
    inline int4 wxyz() const { return {w, x, y, z}; }
    inline void set_wxyz(const int4& v) { w = v.x; x = v.y; y = v.z; z = v.w; }
    inline int4 wxyw() const { return {w, x, y, w}; }
    inline int4 wxzx() const { return {w, x, z, x}; }
    inline int4 wxzy() const { return {w, x, z, y}; }
    inline void set_wxzy(const int4& v) { w = v.x; x = v.y; z = v.z; y = v.w; }
    inline int4 wxzz() const { return {w, x, z, z}; }
    inline int4 wxzw() const { return {w, x, z, w}; }
    inline int4 wxwx() const { return {w, x, w, x}; }
    inline int4 wxwy() const { return {w, x, w, y}; }
    inline int4 wxwz() const { return {w, x, w, z}; }
    inline int4 wxww() const { return {w, x, w, w}; }
    inline int4 wyxx() const { return {w, y, x, x}; }
    inline int4 wyxy() const { return {w, y, x, y}; }
    inline int4 wyxz() const { return {w, y, x, z}; }
    inline void set_wyxz(const int4& v) { w = v.x; y = v.y; x = v.z; z = v.w; }
    inline int4 wyxw() const { return {w, y, x, w}; }
    inline int4 wyyx() const { return {w, y, y, x}; }
    inline int4 wyyy() const { return {w, y, y, y}; }
    inline int4 wyyz() const { return {w, y, y, z}; }
    inline int4 wyyw() const { return {w, y, y, w}; }
    inline int4 wyzx() const { return {w, y, z, x}; }
    inline void set_wyzx(const int4& v) { w = v.x; y = v.y; z = v.z; x = v.w; }
    inline int4 wyzy() const { return {w, y, z, y}; }
    inline int4 wyzz() const { return {w, y, z, z}; }
    inline int4 wyzw() const { return {w, y, z, w}; }
    inline int4 wywx() const { return {w, y, w, x}; }
    inline int4 wywy() const { return {w, y, w, y}; }
    inline int4 wywz() const { return {w, y, w, z}; }
    inline int4 wyww() const { return {w, y, w, w}; }
    inline int4 wzxx() const { return {w, z, x, x}; }
    inline int4 wzxy() const { return {w, z, x, y}; }
    inline void set_wzxy(const int4& v) { w = v.x; z = v.y; x = v.z; y = v.w; }
    inline int4 wzxz() const { return {w, z, x, z}; }
    inline int4 wzxw() const { return {w, z, x, w}; }
    inline int4 wzyx() const { return {w, z, y, x}; }
    inline void set_wzyx(const int4& v) { w = v.x; z = v.y; y = v.z; x = v.w; }
    inline int4 wzyy() const { return {w, z, y, y}; }
    inline int4 wzyz() const { return {w, z, y, z}; }
    inline int4 wzyw() const { return {w, z, y, w}; }
    inline int4 wzzx() const { return {w, z, z, x}; }
    inline int4 wzzy() const { return {w, z, z, y}; }
    inline int4 wzzz() const { return {w, z, z, z}; }
    inline int4 wzzw() const { return {w, z, z, w}; }
    inline int4 wzwx() const { return {w, z, w, x}; }
    inline int4 wzwy() const { return {w, z, w, y}; }
    inline int4 wzwz() const { return {w, z, w, z}; }
    inline int4 wzww() const { return {w, z, w, w}; }
    inline int4 wwxx() const { return {w, w, x, x}; }
    inline int4 wwxy() const { return {w, w, x, y}; }
    inline int4 wwxz() const { return {w, w, x, z}; }
    inline int4 wwxw() const { return {w, w, x, w}; }
    inline int4 wwyx() const { return {w, w, y, x}; }
    inline int4 wwyy() const { return {w, w, y, y}; }
    inline int4 wwyz() const { return {w, w, y, z}; }
    inline int4 wwyw() const { return {w, w, y, w}; }
    inline int4 wwzx() const { return {w, w, z, x}; }
    inline int4 wwzy() const { return {w, w, z, y}; }
    inline int4 wwzz() const { return {w, w, z, z}; }
    inline int4 wwzw() const { return {w, w, z, w}; }
    inline int4 wwwx() const { return {w, w, w, x}; }
    inline int4 wwwy() const { return {w, w, w, y}; }
    inline int4 wwwz() const { return {w, w, w, z}; }
    inline int4 wwww() const { return {w, w, w, w}; }
    
    // hash
    inline static size_t _skr_hash(const int4& v) {
        auto hasher = ::skr::Hash<int32_t>{};
        auto result = hasher(v.x);
        result = ::skr::hash_combine(result, hasher(v.y));
        result = ::skr::hash_combine(result, hasher(v.z));
        result = ::skr::hash_combine(result, hasher(v.w));
        return result;
    }
};
}
}
