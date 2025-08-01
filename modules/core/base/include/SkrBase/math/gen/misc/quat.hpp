//! *************************************************************************
//! **  This file is auto-generated by gen_math, do not edit it manually.  **
//! *************************************************************************

#pragma once
#include "../vec/gen_vector.hpp"
#include "../mat/gen_matrix.hpp"
#include "../math/gen_math_func.hpp"

namespace skr {
inline namespace math {
struct alignas(16) QuatF {
    float x, y, z, w;
    
    // ctor & dtor
    inline QuatF() : x(0), y(0), z(0), w(1) {}
    inline QuatF(MathNoInitType) {}
    inline QuatF(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    inline ~QuatF() = default;
    
    // factory
    inline static QuatF Identity() { return { 0, 0, 0, 1 }; }
    inline static QuatF Fill(float v) { return { v, v, v, v }; }
    static QuatF Euler(float pitch, float yaw, float roll);
    static QuatF AxisAngle(float3 axis, float angle);
    
    // copy & move & assign & move assign
    inline QuatF(QuatF const&) = default;
    inline QuatF(QuatF&&) = default;
    inline QuatF& operator=(QuatF const&) = default;
    inline QuatF& operator=(QuatF&&) = default;
    
    // convert with vector
    inline explicit QuatF(const float4& vec) : x(vec.x), y(vec.y), z(vec.z), w(vec.w) {}
    inline explicit operator float4() const { return { x, y, z, w }; }
    
    // as vector
    inline float4& as_vector() { return *reinterpret_cast<float4*>(this); }
    inline const float4& as_vector() const { return *reinterpret_cast<const float4*>(this); }
    
    // convert with rotator
    explicit QuatF(const RotatorF& rotator);
    static QuatF FromRotator(const RotatorF& rotator);
    
    // negative operator
    QuatF operator-() const;
    
    // mul assign operator
    QuatF& operator*=(const QuatF& rhs);
    
    // get axis & angle
    float3 axis() const;
    float angle() const;
    void axis_angle(float3& axis, float& angle) const;
    
    // identity
    bool is_identity() const;
    bool is_nearly_identity(float threshold_angle = float(0.00284714461)) const;
    
    // to matrix
    operator float3x3() const;
    operator float4x4() const;
    float3x3 to_matrix3() const;
    float4x4 to_matrix4() const;
    
    // from matrix
    QuatF(const float3x3& mat);
    QuatF(const float4x4& mat);
    static QuatF FromMatrix(const float3x3& mat);
    static QuatF FromMatrix(const float4x4& mat);
};
struct alignas(16) QuatD {
    double x, y, z, w;
    
    // ctor & dtor
    inline QuatD() : x(0), y(0), z(0), w(1) {}
    inline QuatD(MathNoInitType) {}
    inline QuatD(double x, double y, double z, double w) : x(x), y(y), z(z), w(w) {}
    inline ~QuatD() = default;
    
    // factory
    inline static QuatD Identity() { return { 0, 0, 0, 1 }; }
    inline static QuatD Fill(double v) { return { v, v, v, v }; }
    static QuatD Euler(double pitch, double yaw, double roll);
    static QuatD AxisAngle(double3 axis, double angle);
    
    // copy & move & assign & move assign
    inline QuatD(QuatD const&) = default;
    inline QuatD(QuatD&&) = default;
    inline QuatD& operator=(QuatD const&) = default;
    inline QuatD& operator=(QuatD&&) = default;
    
    // convert with vector
    inline explicit QuatD(const double4& vec) : x(vec.x), y(vec.y), z(vec.z), w(vec.w) {}
    inline explicit operator double4() const { return { x, y, z, w }; }
    
    // as vector
    inline double4& as_vector() { return *reinterpret_cast<double4*>(this); }
    inline const double4& as_vector() const { return *reinterpret_cast<const double4*>(this); }
    
    // convert with rotator
    explicit QuatD(const RotatorD& rotator);
    static QuatD FromRotator(const RotatorD& rotator);
    
    // negative operator
    QuatD operator-() const;
    
    // mul assign operator
    QuatD& operator*=(const QuatD& rhs);
    
    // get axis & angle
    double3 axis() const;
    double angle() const;
    void axis_angle(double3& axis, double& angle) const;
    
    // identity
    bool is_identity() const;
    bool is_nearly_identity(double threshold_angle = double(0.00284714461)) const;
    
    // to matrix
    operator double3x3() const;
    operator double4x4() const;
    double3x3 to_matrix3() const;
    double4x4 to_matrix4() const;
    
    // from matrix
    QuatD(const double3x3& mat);
    QuatD(const double4x4& mat);
    static QuatD FromMatrix(const double3x3& mat);
    static QuatD FromMatrix(const double4x4& mat);
};
}
}
