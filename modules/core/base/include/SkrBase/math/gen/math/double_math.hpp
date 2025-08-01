//! *************************************************************************
//! **  This file is auto-generated by gen_math, do not edit it manually.  **
//! *************************************************************************

#pragma once
#include "../vec/bool_vec.hpp"
#include "../vec/double_vec.hpp"
#include "../../math_constants.hpp"

namespace skr {
inline namespace math {
// abs
double abs(double v);
double2 abs(const double2& v);
double3 abs(const double3& v);
double4 abs(const double4& v);

// acos
double acos(double v);
double2 acos(const double2& v);
double3 acos(const double3& v);
double4 acos(const double4& v);

// acosh
inline double acosh(double v) { return ::std::acosh(v); }
inline double2 acosh(const double2& v) { return {::std::acosh(v.x), ::std::acosh(v.y)}; }
inline double3 acosh(const double3& v) { return {::std::acosh(v.x), ::std::acosh(v.y), ::std::acosh(v.z)}; }
inline double4 acosh(const double4& v) { return {::std::acosh(v.x), ::std::acosh(v.y), ::std::acosh(v.z), ::std::acosh(v.w)}; }

// cos
double cos(double v);
double2 cos(const double2& v);
double3 cos(const double3& v);
double4 cos(const double4& v);

// cosh
inline double cosh(double v) { return ::std::cosh(v); }
inline double2 cosh(const double2& v) { return {::std::cosh(v.x), ::std::cosh(v.y)}; }
inline double3 cosh(const double3& v) { return {::std::cosh(v.x), ::std::cosh(v.y), ::std::cosh(v.z)}; }
inline double4 cosh(const double4& v) { return {::std::cosh(v.x), ::std::cosh(v.y), ::std::cosh(v.z), ::std::cosh(v.w)}; }

// asin
double asin(double v);
double2 asin(const double2& v);
double3 asin(const double3& v);
double4 asin(const double4& v);

// asinh
inline double asinh(double v) { return ::std::asinh(v); }
inline double2 asinh(const double2& v) { return {::std::asinh(v.x), ::std::asinh(v.y)}; }
inline double3 asinh(const double3& v) { return {::std::asinh(v.x), ::std::asinh(v.y), ::std::asinh(v.z)}; }
inline double4 asinh(const double4& v) { return {::std::asinh(v.x), ::std::asinh(v.y), ::std::asinh(v.z), ::std::asinh(v.w)}; }

// sin
double sin(double v);
double2 sin(const double2& v);
double3 sin(const double3& v);
double4 sin(const double4& v);

// sinh
inline double sinh(double v) { return ::std::sinh(v); }
inline double2 sinh(const double2& v) { return {::std::sinh(v.x), ::std::sinh(v.y)}; }
inline double3 sinh(const double3& v) { return {::std::sinh(v.x), ::std::sinh(v.y), ::std::sinh(v.z)}; }
inline double4 sinh(const double4& v) { return {::std::sinh(v.x), ::std::sinh(v.y), ::std::sinh(v.z), ::std::sinh(v.w)}; }

// atan
double atan(double v);
double2 atan(const double2& v);
double3 atan(const double3& v);
double4 atan(const double4& v);

// atanh
inline double atanh(double v) { return ::std::atanh(v); }
inline double2 atanh(const double2& v) { return {::std::atanh(v.x), ::std::atanh(v.y)}; }
inline double3 atanh(const double3& v) { return {::std::atanh(v.x), ::std::atanh(v.y), ::std::atanh(v.z)}; }
inline double4 atanh(const double4& v) { return {::std::atanh(v.x), ::std::atanh(v.y), ::std::atanh(v.z), ::std::atanh(v.w)}; }

// tan
double tan(double v);
double2 tan(const double2& v);
double3 tan(const double3& v);
double4 tan(const double4& v);

// tanh
inline double tanh(double v) { return ::std::tanh(v); }
inline double2 tanh(const double2& v) { return {::std::tanh(v.x), ::std::tanh(v.y)}; }
inline double3 tanh(const double3& v) { return {::std::tanh(v.x), ::std::tanh(v.y), ::std::tanh(v.z)}; }
inline double4 tanh(const double4& v) { return {::std::tanh(v.x), ::std::tanh(v.y), ::std::tanh(v.z), ::std::tanh(v.w)}; }

// sincos
void sincos(double v, double& out_sin, double& out_cos);
void sincos(const double2& v, double2& out_sin, double2& out_cos);
void sincos(const double3& v, double3& out_sin, double3& out_cos);
void sincos(const double4& v, double4& out_sin, double4& out_cos);

// atan2
double atan2(double y, double x);
double2 atan2(const double2& y, const double2& x);
double3 atan2(const double3& y, const double3& x);
double4 atan2(const double4& y, const double4& x);

// ceil
double ceil(double v);
double2 ceil(const double2& v);
double3 ceil(const double3& v);
double4 ceil(const double4& v);

// floor
double floor(double v);
double2 floor(const double2& v);
double3 floor(const double3& v);
double4 floor(const double4& v);

// round
double round(double v);
double2 round(const double2& v);
double3 round(const double3& v);
double4 round(const double4& v);

// trunc
inline double trunc(double v) { return ::std::trunc(v); }
inline double2 trunc(const double2& v) { return {::std::trunc(v.x), ::std::trunc(v.y)}; }
inline double3 trunc(const double3& v) { return {::std::trunc(v.x), ::std::trunc(v.y), ::std::trunc(v.z)}; }
inline double4 trunc(const double4& v) { return {::std::trunc(v.x), ::std::trunc(v.y), ::std::trunc(v.z), ::std::trunc(v.w)}; }

// frac
inline double frac(double v) { double int_ptr; return ::std::modf(v, &int_ptr); }
inline double2 frac(const double2& v) { double2 int_ptr; return {::std::modf(v.x, &int_ptr.x), ::std::modf(v.y, &int_ptr.y)}; }
inline double3 frac(const double3& v) { double3 int_ptr; return {::std::modf(v.x, &int_ptr.x), ::std::modf(v.y, &int_ptr.y), ::std::modf(v.z, &int_ptr.z)}; }
inline double4 frac(const double4& v) { double4 int_ptr; return {::std::modf(v.x, &int_ptr.x), ::std::modf(v.y, &int_ptr.y), ::std::modf(v.z, &int_ptr.z), ::std::modf(v.w, &int_ptr.w)}; }

// modf
inline double modf(double v, double& int_part) { return ::std::modf(v, &int_part); }
inline double2 modf(const double2& v, double2& int_part) { return { ::std::modf(v.x, &int_part.x), ::std::modf(v.y, &int_part.y) }; }
inline double3 modf(const double3& v, double3& int_part) { return { ::std::modf(v.x, &int_part.x), ::std::modf(v.y, &int_part.y), ::std::modf(v.z, &int_part.z) }; }
inline double4 modf(const double4& v, double4& int_part) { return { ::std::modf(v.x, &int_part.x), ::std::modf(v.y, &int_part.y), ::std::modf(v.z, &int_part.z), ::std::modf(v.w, &int_part.w) }; }

// fmod
inline double fmod(double x, double y) { return ::std::fmod(x, y); }
inline double2 fmod(const double2& x, const double2& y) { return {::std::fmod(x.x, y.x), ::std::fmod(x.y, y.y)}; }
inline double3 fmod(const double3& x, const double3& y) { return {::std::fmod(x.x, y.x), ::std::fmod(x.y, y.y), ::std::fmod(x.z, y.z)}; }
inline double4 fmod(const double4& x, const double4& y) { return {::std::fmod(x.x, y.x), ::std::fmod(x.y, y.y), ::std::fmod(x.z, y.z), ::std::fmod(x.w, y.w)}; }

// exp
inline double exp(double v) { return ::std::exp(v); }
inline double2 exp(const double2& v) { return {::std::exp(v.x), ::std::exp(v.y)}; }
inline double3 exp(const double3& v) { return {::std::exp(v.x), ::std::exp(v.y), ::std::exp(v.z)}; }
inline double4 exp(const double4& v) { return {::std::exp(v.x), ::std::exp(v.y), ::std::exp(v.z), ::std::exp(v.w)}; }

// exp2
inline double exp2(double v) { return ::std::exp2(v); }
inline double2 exp2(const double2& v) { return {::std::exp2(v.x), ::std::exp2(v.y)}; }
inline double3 exp2(const double3& v) { return {::std::exp2(v.x), ::std::exp2(v.y), ::std::exp2(v.z)}; }
inline double4 exp2(const double4& v) { return {::std::exp2(v.x), ::std::exp2(v.y), ::std::exp2(v.z), ::std::exp2(v.w)}; }

// log
inline double log(double v) { return ::std::log(v); }
inline double2 log(const double2& v) { return {::std::log(v.x), ::std::log(v.y)}; }
inline double3 log(const double3& v) { return {::std::log(v.x), ::std::log(v.y), ::std::log(v.z)}; }
inline double4 log(const double4& v) { return {::std::log(v.x), ::std::log(v.y), ::std::log(v.z), ::std::log(v.w)}; }

// log2
inline double log2(double v) { return ::std::log2(v); }
inline double2 log2(const double2& v) { return {::std::log2(v.x), ::std::log2(v.y)}; }
inline double3 log2(const double3& v) { return {::std::log2(v.x), ::std::log2(v.y), ::std::log2(v.z)}; }
inline double4 log2(const double4& v) { return {::std::log2(v.x), ::std::log2(v.y), ::std::log2(v.z), ::std::log2(v.w)}; }

// log10
inline double log10(double v) { return ::std::log10(v); }
inline double2 log10(const double2& v) { return {::std::log10(v.x), ::std::log10(v.y)}; }
inline double3 log10(const double3& v) { return {::std::log10(v.x), ::std::log10(v.y), ::std::log10(v.z)}; }
inline double4 log10(const double4& v) { return {::std::log10(v.x), ::std::log10(v.y), ::std::log10(v.z), ::std::log10(v.w)}; }

// logx
inline double logx(double v, double base) { return ::std::log(v) / ::std::log(base); }
inline double2 logx(const double2& v, const double2& base) { return {logx(v.x, base.x), logx(v.y, base.y)}; }
inline double3 logx(const double3& v, const double3& base) { return {logx(v.x, base.x), logx(v.y, base.y), logx(v.z, base.z)}; }
inline double4 logx(const double4& v, const double4& base) { return {logx(v.x, base.x), logx(v.y, base.y), logx(v.z, base.z), logx(v.w, base.w)}; }

// isfinite
inline bool isfinite(double v) { return ::std::isfinite(v); }
inline bool2 isfinite(const double2& v) { return {::std::isfinite(v.x), ::std::isfinite(v.y)}; }
inline bool3 isfinite(const double3& v) { return {::std::isfinite(v.x), ::std::isfinite(v.y), ::std::isfinite(v.z)}; }
inline bool4 isfinite(const double4& v) { return {::std::isfinite(v.x), ::std::isfinite(v.y), ::std::isfinite(v.z), ::std::isfinite(v.w)}; }

// isinf
inline bool isinf(double v) { return ::std::isinf(v); }
inline bool2 isinf(const double2& v) { return {::std::isinf(v.x), ::std::isinf(v.y)}; }
inline bool3 isinf(const double3& v) { return {::std::isinf(v.x), ::std::isinf(v.y), ::std::isinf(v.z)}; }
inline bool4 isinf(const double4& v) { return {::std::isinf(v.x), ::std::isinf(v.y), ::std::isinf(v.z), ::std::isinf(v.w)}; }

// isnan
inline bool isnan(double v) { return ::std::isnan(v); }
inline bool2 isnan(const double2& v) { return {::std::isnan(v.x), ::std::isnan(v.y)}; }
inline bool3 isnan(const double3& v) { return {::std::isnan(v.x), ::std::isnan(v.y), ::std::isnan(v.z)}; }
inline bool4 isnan(const double4& v) { return {::std::isnan(v.x), ::std::isnan(v.y), ::std::isnan(v.z), ::std::isnan(v.w)}; }

// max
double max(double v1, double v2);
double2 max(const double2& v1, const double2& v2);
double3 max(const double3& v1, const double3& v2);
double4 max(const double4& v1, const double4& v2);

// min
double min(double v1, double v2);
double2 min(const double2& v1, const double2& v2);
double3 min(const double3& v1, const double3& v2);
double4 min(const double4& v1, const double4& v2);

// mad
inline double mad(double x, double mul, double add) { return ::std::fma(x, mul, add); }
inline double2 mad(const double2& x, const double2& mul, const double2& add) { return {::std::fma(x.x, mul.x, add.x), ::std::fma(x.y, mul.y, add.y)}; }
inline double3 mad(const double3& x, const double3& mul, const double3& add) { return {::std::fma(x.x, mul.x, add.x), ::std::fma(x.y, mul.y, add.y), ::std::fma(x.z, mul.z, add.z)}; }
inline double4 mad(const double4& x, const double4& mul, const double4& add) { return {::std::fma(x.x, mul.x, add.x), ::std::fma(x.y, mul.y, add.y), ::std::fma(x.z, mul.z, add.z), ::std::fma(x.w, mul.w, add.w)}; }

// pow
inline double pow(double x, double y) { return ::std::pow(x, y); }
inline double2 pow(const double2& x, const double2& y) { return {::std::pow(x.x, y.x), ::std::pow(x.y, y.y)}; }
inline double3 pow(const double3& x, const double3& y) { return {::std::pow(x.x, y.x), ::std::pow(x.y, y.y), ::std::pow(x.z, y.z)}; }
inline double4 pow(const double4& x, const double4& y) { return {::std::pow(x.x, y.x), ::std::pow(x.y, y.y), ::std::pow(x.z, y.z), ::std::pow(x.w, y.w)}; }

// sqrt
double sqrt(double v);
double2 sqrt(const double2& v);
double3 sqrt(const double3& v);
double4 sqrt(const double4& v);

// rsqrt
double rsqrt(double v);
double2 rsqrt(const double2& v);
double3 rsqrt(const double3& v);
double4 rsqrt(const double4& v);

// cbrt
inline double cbrt(double v) { return ::std::cbrt(v); }
inline double2 cbrt(const double2& v) { return {::std::cbrt(v.x), ::std::cbrt(v.y)}; }
inline double3 cbrt(const double3& v) { return {::std::cbrt(v.x), ::std::cbrt(v.y), ::std::cbrt(v.z)}; }
inline double4 cbrt(const double4& v) { return {::std::cbrt(v.x), ::std::cbrt(v.y), ::std::cbrt(v.z), ::std::cbrt(v.w)}; }

// hypot
inline double hypot(double x, double y) { return ::std::hypot(x, y); }
inline double hypot(double x, double y, double z) { return ::std::hypot(x, y, z); }
inline double2 hypot(const double2& x, const double2& y) { return {::std::hypot(x.x, y.x), ::std::hypot(x.y, y.y)}; }
inline double2 hypot(const double2& x, const double2& y, const double2& z) { return {::std::hypot(x.x, y.x, z.x), ::std::hypot(x.y, y.y, z.y)}; }
inline double3 hypot(const double3& x, const double3& y) { return {::std::hypot(x.x, y.x), ::std::hypot(x.y, y.y), ::std::hypot(x.z, y.z)}; }
inline double3 hypot(const double3& x, const double3& y, const double3& z) { return {::std::hypot(x.x, y.x, z.x), ::std::hypot(x.y, y.y, z.y), ::std::hypot(x.z, y.z, z.z)}; }
inline double4 hypot(const double4& x, const double4& y) { return {::std::hypot(x.x, y.x), ::std::hypot(x.y, y.y), ::std::hypot(x.z, y.z), ::std::hypot(x.w, y.w)}; }
inline double4 hypot(const double4& x, const double4& y, const double4& z) { return {::std::hypot(x.x, y.x, z.x), ::std::hypot(x.y, y.y, z.y), ::std::hypot(x.z, y.z, z.z), ::std::hypot(x.w, y.w, z.w)}; }

// clamp
inline double clamp(double v, double min, double max) { return v < min ? min : v > max ? max : v; }
inline double2 clamp(const double2& v, const double2& min, const double2& max) { return {clamp(v.x, min.x, max.x), clamp(v.y, min.y, max.y)}; }
inline double3 clamp(const double3& v, const double3& min, const double3& max) { return {clamp(v.x, min.x, max.x), clamp(v.y, min.y, max.y), clamp(v.z, min.z, max.z)}; }
inline double4 clamp(const double4& v, const double4& min, const double4& max) { return {clamp(v.x, min.x, max.x), clamp(v.y, min.y, max.y), clamp(v.z, min.z, max.z), clamp(v.w, min.w, max.w)}; }

// saturate
inline double saturate(double v) { return clamp(v, double(0), double(1)); }
inline double2 saturate(const double2& v) { return {clamp(v.x, double(0), double(1)), clamp(v.y, double(0), double(1))}; }
inline double3 saturate(const double3& v) { return {clamp(v.x, double(0), double(1)), clamp(v.y, double(0), double(1)), clamp(v.z, double(0), double(1))}; }
inline double4 saturate(const double4& v) { return {clamp(v.x, double(0), double(1)), clamp(v.y, double(0), double(1)), clamp(v.z, double(0), double(1)), clamp(v.w, double(0), double(1))}; }

// select
inline double2 select(bool2 c, double2 if_true, double2 if_false) { return { c.x ? if_true.x : if_false.x, c.y ? if_true.y : if_false.y }; }
inline double3 select(bool3 c, double3 if_true, double3 if_false) { return { c.x ? if_true.x : if_false.x, c.y ? if_true.y : if_false.y, c.z ? if_true.z : if_false.z }; }
inline double4 select(bool4 c, double4 if_true, double4 if_false) { return { c.x ? if_true.x : if_false.x, c.y ? if_true.y : if_false.y, c.z ? if_true.z : if_false.z, c.w ? if_true.w : if_false.w }; }

// rcp
double rcp(double v);
double2 rcp(const double2& v);
double3 rcp(const double3& v);
double4 rcp(const double4& v);

// sign
inline double sign(double v) { return v < double(0) ? double(-1) : v > double(0) ? double(1) : double(0); }
inline double2 sign(const double2& v) { return {v.x < double(0) ? double(-1) : v.x > double(0) ? double(1) : double(0), v.y < double(0) ? double(-1) : v.y > double(0) ? double(1) : double(0)}; }
inline double3 sign(const double3& v) { return {v.x < double(0) ? double(-1) : v.x > double(0) ? double(1) : double(0), v.y < double(0) ? double(-1) : v.y > double(0) ? double(1) : double(0), v.z < double(0) ? double(-1) : v.z > double(0) ? double(1) : double(0)}; }
inline double4 sign(const double4& v) { return {v.x < double(0) ? double(-1) : v.x > double(0) ? double(1) : double(0), v.y < double(0) ? double(-1) : v.y > double(0) ? double(1) : double(0), v.z < double(0) ? double(-1) : v.z > double(0) ? double(1) : double(0), v.w < double(0) ? double(-1) : v.w > double(0) ? double(1) : double(0)}; }

// degrees
inline double degrees(double v) { return v * double(180) / double(kPi); }
inline double2 degrees(const double2& v) { return {v.x * double(180) / double(kPi), v.y * double(180) / double(kPi)}; }
inline double3 degrees(const double3& v) { return {v.x * double(180) / double(kPi), v.y * double(180) / double(kPi), v.z * double(180) / double(kPi)}; }
inline double4 degrees(const double4& v) { return {v.x * double(180) / double(kPi), v.y * double(180) / double(kPi), v.z * double(180) / double(kPi), v.w * double(180) / double(kPi)}; }

// radians
inline double radians(double v) { return v * double(kPi) / double(180); }
inline double2 radians(const double2& v) { return {v.x * double(kPi) / double(180), v.y * double(kPi) / double(180)}; }
inline double3 radians(const double3& v) { return {v.x * double(kPi) / double(180), v.y * double(kPi) / double(180), v.z * double(kPi) / double(180)}; }
inline double4 radians(const double4& v) { return {v.x * double(kPi) / double(180), v.y * double(kPi) / double(180), v.z * double(kPi) / double(180), v.w * double(kPi) / double(180)}; }

// dot
double dot(const double2& v1, const double2& v2);
double dot(const double3& v1, const double3& v2);
double dot(const double4& v1, const double4& v2);

// cross
double3 cross(const double3& x, const double3& y);

// length
double length(const double2& v);
double length(const double3& v);
double length(const double4& v);

// length_squared
double length_squared(const double2& v);
double length_squared(const double3& v);
double length_squared(const double4& v);

// rlength
double rlength(const double2& v);
double rlength(const double3& v);
double rlength(const double4& v);

// distance
inline double distance(const double2& x, const double2& y) { return length_squared(y - x); }
inline double distance(const double3& x, const double3& y) { return length_squared(y - x); }
inline double distance(const double4& x, const double4& y) { return length_squared(y - x); }

// distance_squared
inline double distance_squared(const double2& x, const double2& y) { return length(y - x); }
inline double distance_squared(const double3& x, const double3& y) { return length(y - x); }
inline double distance_squared(const double4& x, const double4& y) { return length(y - x); }

// normalize
double2 normalize(const double2& v);
double3 normalize(const double3& v);
double4 normalize(const double4& v);

// reflect
inline double2 reflect(const double2& v, const double2& n) { return v - 2 * dot(n, v) * n; }
inline double3 reflect(const double3& v, const double3& n) { return v - 2 * dot(n, v) * n; }

// refract
inline double2 refract(const double2& v, const double2& n, double eta) {
    const double cos_i = dot(-v, n);
    const double cos_t2 = double(1) - eta * eta * (double(1) - cos_i * cos_i);
    const double2 t = eta * v + (eta * cos_i - ::std::sqrt(::std::abs(cos_t2))) * n;
    return t * double2(cos_t2 > 0);
}
inline double3 refract(const double3& v, const double3& n, double eta) {
    const double cos_i = dot(-v, n);
    const double cos_t2 = double(1) - eta * eta * (double(1) - cos_i * cos_i);
    const double3 t = eta * v + (eta * cos_i - ::std::sqrt(::std::abs(cos_t2))) * n;
    return t * double3(cos_t2 > 0);
}

// step
inline double step(double edge, double v) { return v < edge ? double(0) : double(1); }
inline double2 step(const double2& edge, const double2& v) { return select(edge < v, double2(0), double2(1)); }
inline double3 step(const double3& edge, const double3& v) { return select(edge < v, double3(0), double3(1)); }
inline double4 step(const double4& edge, const double4& v) { return select(edge < v, double4(0), double4(1)); }

// smoothstep
inline double smoothstep(double edge0, double edge1, double v) {
    const double t = saturate((v - edge0) / (edge1 - edge0));
    return t * t * (double(3) - double(2) * v);
}
inline double2 smoothstep(const double2& edge0, const double2& edge1, const double2& v) {
    const double2 t = saturate((v - edge0) / (edge1 - edge0));
    return t * t * (double(3) - double(2) * v);
}
inline double3 smoothstep(const double3& edge0, const double3& edge1, const double3& v) {
    const double3 t = saturate((v - edge0) / (edge1 - edge0));
    return t * t * (double(3) - double(2) * v);
}
inline double4 smoothstep(const double4& edge0, const double4& edge1, const double4& v) {
    const double4 t = saturate((v - edge0) / (edge1 - edge0));
    return t * t * (double(3) - double(2) * v);
}

// lerp
inline double lerp(double v0, double v1, double t) { return v0 + t * (v1 - v0); }
inline double2 lerp(const double2& v0, const double2& v1, double t) { return v0 + t * (v1 - v0); }
double3 lerp(const double3& v0, const double3& v1, double t);
double4 lerp(const double4& v0, const double4& v1, double t);

// nearly_equal
inline bool nearly_equal(double x, double y, double epsilon = double(1.e-4)) { return abs(x - y) <= epsilon; }
inline bool2 nearly_equal(const double2& x, const double2& y, double epsilon = double(1.e-4)) { return { (abs(x.x - y.x) <= epsilon), (abs(x.y - y.y) <= epsilon) }; }
inline bool3 nearly_equal(const double3& x, const double3& y, double epsilon = double(1.e-4)) { return { (abs(x.x - y.x) <= epsilon), (abs(x.y - y.y) <= epsilon), (abs(x.z - y.z) <= epsilon) }; }
inline bool4 nearly_equal(const double4& x, const double4& y, double epsilon = double(1.e-4)) { return { (abs(x.x - y.x) <= epsilon), (abs(x.y - y.y) <= epsilon), (abs(x.z - y.z) <= epsilon), (abs(x.w - y.w) <= epsilon) }; }

// clamp_radians
inline double clamp_radians(double v) {
    v = fmod(v, double(kPi2));
    if (v < double(0)) {
        v += double(kPi2);
    }
    return v;
}
inline double2 clamp_radians(const double2& v) { return { clamp_radians(v.x), clamp_radians(v.y) }; }
inline double3 clamp_radians(const double3& v) { return { clamp_radians(v.x), clamp_radians(v.y), clamp_radians(v.z) }; }
inline double4 clamp_radians(const double4& v) { return { clamp_radians(v.x), clamp_radians(v.y), clamp_radians(v.z), clamp_radians(v.w) }; }

// clamp_degrees
inline double clamp_degrees(double v) {
    v = fmod(v, double(360));
    if (v < double(0)) {
        v += double(360);
    }
    return v;
}
inline double2 clamp_degrees(const double2& v) { return { clamp_degrees(v.x), clamp_degrees(v.y) }; }
inline double3 clamp_degrees(const double3& v) { return { clamp_degrees(v.x), clamp_degrees(v.y), clamp_degrees(v.z) }; }
inline double4 clamp_degrees(const double4& v) { return { clamp_degrees(v.x), clamp_degrees(v.y), clamp_degrees(v.z), clamp_degrees(v.w) }; }

// normalize_radians
inline double normalize_radians(double v) {
    v = clamp_radians(v);
    if (v > double(kPi)) {
        v -= double(kPi2);
    }
    return v;
}
inline double2 normalize_radians(const double2& v) { return { normalize_radians(v.x), normalize_radians(v.y) }; }
inline double3 normalize_radians(const double3& v) { return { normalize_radians(v.x), normalize_radians(v.y), normalize_radians(v.z) }; }
inline double4 normalize_radians(const double4& v) { return { normalize_radians(v.x), normalize_radians(v.y), normalize_radians(v.z), normalize_radians(v.w) }; }

// normalize_degrees
inline double normalize_degrees(double v) {
    v = clamp_degrees(v);
    if (v > double(180)) {
        v -= double(360);
    }
    return v;
}
inline double2 normalize_degrees(const double2& v) { return { normalize_degrees(v.x), normalize_degrees(v.y) }; }
inline double3 normalize_degrees(const double3& v) { return { normalize_degrees(v.x), normalize_degrees(v.y), normalize_degrees(v.z) }; }
inline double4 normalize_degrees(const double4& v) { return { normalize_degrees(v.x), normalize_degrees(v.y), normalize_degrees(v.z), normalize_degrees(v.w) }; }

// square
inline double square(double v) { return v * v; }
inline double2 square(const double2& v) { return { v.x * v.x, v.y * v.y }; }
inline double3 square(const double3& v) { return { v.x * v.x, v.y * v.y, v.z * v.z }; }
inline double4 square(const double4& v) { return { v.x * v.x, v.y * v.y, v.z * v.z, v.w * v.w }; }

// cube
inline double cube(double v) { return v * v * v; }
inline double2 cube(const double2& v) { return { v.x * v.x * v.x, v.y * v.y * v.y }; }
inline double3 cube(const double3& v) { return { v.x * v.x * v.x, v.y * v.y * v.y, v.z * v.z * v.z }; }
inline double4 cube(const double4& v) { return { v.x * v.x * v.x, v.y * v.y * v.y, v.z * v.z * v.z, v.w * v.w * v.w }; }

}
}
