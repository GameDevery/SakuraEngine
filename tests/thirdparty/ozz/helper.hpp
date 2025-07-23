#pragma once
#include "SkrTestFramework/framework.hpp"
#include "SkrAnim/ozz/base/maths/soa_float.h"
#include "SkrAnim/ozz/base/maths/soa_float4x4.h"

namespace ozz
{

using namespace ozz::math;

inline void check_eq(const ozz::math::SimdFloat4& u, const float u0, const float u1, const float u2, const float u3, const float epsilon = 1e-6f)
{
    union
    {
        ozz::math::SimdFloat4 simd;
        float                 values[4];
    } c = { u };
    CHECK(c.values[0] == doctest::Approx(u0).epsilon(epsilon));
    CHECK(c.values[1] == doctest::Approx(u1).epsilon(epsilon));
    CHECK(c.values[2] == doctest::Approx(u2).epsilon(epsilon));
    CHECK(c.values[3] == doctest::Approx(u3).epsilon(epsilon));
}

// the four packed float4
inline void check_eq(const ozz::math::SoaFloat4& m, const float x0, const float x1, const float x2, const float x3, const float y0, const float y1, const float y2, const float y3, const float z0, const float z1, const float z2, const float z3, const float w0, const float w1, const float w2, const float w3, const float epsilon = 1e-6f)
{
    check_eq(m.x, x0, x1, x2, x3, epsilon);
    check_eq(m.y, y0, y1, y2, y3, epsilon);
    check_eq(m.z, z0, z1, z2, z3, epsilon);
    check_eq(m.w, w0, w1, w2, w3, epsilon);
}

inline void check_eq(const ozz::math::SoaFloat4x4& m, const float col0xx, const float col0xy, const float col0xz, const float col0xw, const float col0yx, const float col0yy, const float col0yz, const float col0yw, const float col0zx, const float col0zy, const float col0zz, const float col0zw, const float col0wx, const float col0wy, const float col0wz, const float col0ww, const float col1xx, const float col1xy, const float col1xz, const float col1xw, const float col1yx, const float col1yy, const float col1yz, const float col1yw, const float col1zx, const float col1zy, const float col1zz, const float col1zw, const float col1wx, const float col1wy, const float col1wz, const float col1ww, const float col2xx, const float col2xy, const float col2xz, const float col2xw, const float col2yx, const float col2yy, const float col2yz, const float col2yw, const float col2zx, const float col2zy, const float col2zz, const float col2zw, const float col2wx, const float col2wy, const float col2wz, const float col2ww, const float col3xx, const float col3xy, const float col3xz, const float col3xw, const float col3yx, const float col3yy, const float col3yz, const float col3yw, const float col3zx, const float col3zy, const float col3zz, const float col3zw, const float col3wx, const float col3wy, const float col3wz, const float col3ww, const double epsilon = 1e-6f)
{
    check_eq(m.cols[0], col0xx, col0xy, col0xz, col0xw, col0yx, col0yy, col0yz, col0yw, col0zx, col0zy, col0zz, col0zw, col0wx, col0wy, col0wz, col0ww, epsilon);
    check_eq(m.cols[1], col1xx, col1xy, col1xz, col1xw, col1yx, col1yy, col1yz, col1yw, col1zx, col1zy, col1zz, col1zw, col1wx, col1wy, col1wz, col1ww, epsilon);
    check_eq(m.cols[2], col2xx, col2xy, col2xz, col2xw, col2yx, col2yy, col2yz, col2yw, col2zx, col2zy, col2zz, col2zw, col2wx, col2wy, col2wz, col2ww, epsilon);
    check_eq(m.cols[3], col3xx, col3xy, col3xz, col3xw, col3yx, col3yy, col3yz, col3yw, col3zx, col3zy, col3zz, col3zw, col3wx, col3wy, col3wz, col3ww, epsilon);
}

} // namespace ozz