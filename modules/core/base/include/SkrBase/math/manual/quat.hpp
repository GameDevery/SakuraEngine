#pragma once
#include "./rtm_conv.hpp"
#include "../gen/misc/quat.hpp"

namespace skr
{
inline namespace math
{
// impl quat factory
inline QuatF QuatF::Euler(float pitch, float yaw, float roll)
{
    // we just need to match direction  order, then the quat will be correct
    return RtmConvert<QuatF>::from_rtm(
        rtm::quat_from_euler(
            yaw,  // [Right] rtm: pitch->y means right, skr: yaw->y means up
            roll, // [Up] rtm: yaw->z means up, skr: roll->z means forward
            pitch // [Forward] rtm: roll->x means forward, skr: pitch->x means right
        )
    );
}
inline QuatD QuatD::Euler(double pitch, double yaw, double roll)
{
    // we just need to match direction order, then the quat will be correct
    return RtmConvert<QuatD>::from_rtm(
        rtm::quat_from_euler(
            yaw,  // [Right] rtm: pitch->y means [Right], skr: yaw->y means up
            roll, // [Up] rtm: yaw->z means [Up], skr: roll->z means forward
            pitch // [Forward] rtm: roll->x means [Forward], skr: pitch->x means right
        )
    );
}
inline QuatF QuatF::AxisAngle(float3 axis, float angle)
{
    return RtmConvert<QuatF>::from_rtm(
        rtm::quat_from_axis_angle(
            RtmConvert<float3>::to_rtm(axis),
            angle
        )
    );
}
inline QuatD QuatD::AxisAngle(double3 axis, double angle)
{
    return RtmConvert<QuatD>::from_rtm(
        rtm::quat_from_axis_angle(
            RtmConvert<double3>::to_rtm(axis),
            angle
        )
    );
}

// impl negative operator
inline QuatF QuatF::operator-() const
{
    return RtmConvert<QuatF>::from_rtm(
        rtm::quat_neg(
            RtmConvert<QuatF>::to_rtm(*this)
        )
    );
}
inline QuatD QuatD::operator-() const
{
    return RtmConvert<QuatD>::from_rtm(
        rtm::quat_neg(
            RtmConvert<QuatD>::to_rtm(*this)
        )
    );
}

// get axis & angle
inline float3 QuatF::axis() const
{
    return RtmConvert<float3>::from_rtm(
        rtm::quat_get_axis(
            RtmConvert<QuatF>::to_rtm(*this)
        )
    );
}
inline double3 QuatD::axis() const
{
    return RtmConvert<double3>::from_rtm(
        rtm::quat_get_axis(
            RtmConvert<QuatD>::to_rtm(*this)
        )
    );
}
inline float QuatF::angle() const
{
    return rtm::quat_get_angle(
        RtmConvert<QuatF>::to_rtm(*this)
    );
}
inline double QuatD::angle() const
{
    return rtm::quat_get_angle(
        RtmConvert<QuatD>::to_rtm(*this)
    );
}
inline void QuatF::axis_angle(float3& axis, float& angle) const
{
    rtm::vector4f out_axis;
    rtm::quat_to_axis_angle(
        RtmConvert<QuatF>::to_rtm(*this),
        out_axis,
        angle
    );
    RtmConvert<float3>::store(out_axis, axis);
}
inline void QuatD::axis_angle(double3& axis, double& angle) const
{
    rtm::vector4d out_axis;
    rtm::quat_to_axis_angle(
        RtmConvert<QuatD>::to_rtm(*this),
        out_axis,
        angle
    );
    RtmConvert<double3>::store(out_axis, axis);
}

// impl conjugate
inline QuatF conjugate(const QuatF& q)
{
    return RtmConvert<QuatF>::from_rtm(
        rtm ::quat_conjugate(
            RtmConvert<QuatF>::to_rtm(q)
        )
    );
}
inline QuatD conjugate(const QuatD& q)
{
    return RtmConvert<QuatD>::from_rtm(
        rtm ::quat_conjugate(
            RtmConvert<QuatD>::to_rtm(q)
        )
    );
}

// impl quat mul quat, means first apply lhs rotation, then apply rhs rotation
inline QuatF mul(const QuatF& lhs, const QuatF& rhs)
{
    return RtmConvert<QuatF>::from_rtm(
        rtm::quat_mul(
            RtmConvert<QuatF>::to_rtm(lhs),
            RtmConvert<QuatF>::to_rtm(rhs)
        )
    );
}
inline QuatD mul(const QuatD& lhs, const QuatD& rhs)
{
    return RtmConvert<QuatD>::from_rtm(
        rtm::quat_mul(
            RtmConvert<QuatD>::to_rtm(lhs),
            RtmConvert<QuatD>::to_rtm(rhs)
        )
    );
}
inline QuatF operator*(const QuatF& lhs, const QuatF& rhs)
{
    return mul(lhs, rhs);
}
inline QuatD operator*(const QuatD& lhs, const QuatD& rhs)
{
    return mul(lhs, rhs);
}

// impl quat mul vector, vector must be lhs, means apply rotation to vector
inline float3 mul(const float3& lhs, const QuatF& rhs)
{
    return RtmConvert<float3>::from_rtm(
        rtm::quat_mul_vector3(
            RtmConvert<float3>::to_rtm(lhs),
            RtmConvert<QuatF>::to_rtm(rhs)
        )
    );
}
inline double3 mul(const double3& lhs, const QuatD& rhs)
{
    return RtmConvert<double3>::from_rtm(
        rtm::quat_mul_vector3(
            RtmConvert<double3>::to_rtm(lhs),
            RtmConvert<QuatD>::to_rtm(rhs)
        )
    );
}
inline float3 operator*(const float3& lhs, const QuatF& rhs)
{
    return mul(lhs, rhs);
}
inline double3 operator*(const double3& lhs, const QuatD& rhs)
{
    return mul(lhs, rhs);
}

// impl quat dot
inline float dot(const QuatF& lhs, const QuatF& rhs)
{
    return rtm::quat_dot(
        RtmConvert<QuatF>::to_rtm(lhs),
        RtmConvert<QuatF>::to_rtm(rhs)
    );
}
inline double dot(const QuatD& lhs, const QuatD& rhs)
{
    return rtm::quat_dot(
        RtmConvert<QuatD>::to_rtm(lhs),
        RtmConvert<QuatD>::to_rtm(rhs)
    );
}

// impl quat length
inline float length(const QuatF& q)
{
    return rtm::quat_length(
        RtmConvert<QuatF>::to_rtm(q)
    );
}
inline double length(const QuatD& q)
{
    return rtm::quat_length(
        RtmConvert<QuatD>::to_rtm(q)
    );
}

// impl quat length squared
inline float length_squared(const QuatF& q)
{
    return rtm::quat_length_squared(
        RtmConvert<QuatF>::to_rtm(q)
    );
}
inline double length_squared(const QuatD& q)
{
    return rtm::quat_length_squared(
        RtmConvert<QuatD>::to_rtm(q)
    );
}

// impl quat length rcp
inline float length_rcp(const QuatF& q)
{
    return rtm::quat_length_reciprocal(
        RtmConvert<QuatF>::to_rtm(q)
    );
}
inline double length_rcp(const QuatD& q)
{
    return rtm::quat_length_reciprocal(
        RtmConvert<QuatD>::to_rtm(q)
    );
}

// impl quat normalize
inline QuatF normalize(const QuatF& q)
{
    return RtmConvert<QuatF>::from_rtm(
        rtm::quat_normalize(
            RtmConvert<QuatF>::to_rtm(q)
        )
    );
}
inline QuatD normalize(const QuatD& q)
{
    return RtmConvert<QuatD>::from_rtm(
        rtm::quat_normalize(
            RtmConvert<QuatD>::to_rtm(q)
        )
    );
}

// impl quat normalize deterministic
inline QuatF normalize_deterministic(const QuatF& q)
{
    return RtmConvert<QuatF>::from_rtm(
        rtm::quat_normalize_deterministic(
            RtmConvert<QuatF>::to_rtm(q)
        )
    );
}
inline QuatD normalize_deterministic(const QuatD& q)
{
    return RtmConvert<QuatD>::from_rtm(
        rtm::quat_normalize_deterministic(
            RtmConvert<QuatD>::to_rtm(q)
        )
    );
}

// impl quat lerp
inline QuatF lerp(const QuatF& lhs, const QuatF& rhs, float t)
{
    return RtmConvert<QuatF>::from_rtm(
        rtm::quat_lerp(
            RtmConvert<QuatF>::to_rtm(lhs),
            RtmConvert<QuatF>::to_rtm(rhs),
            t
        )
    );
}
inline QuatD lerp(const QuatD& lhs, const QuatD& rhs, double t)
{
    return RtmConvert<QuatD>::from_rtm(
        rtm::quat_lerp(
            RtmConvert<QuatD>::to_rtm(lhs),
            RtmConvert<QuatD>::to_rtm(rhs),
            t
        )
    );
}

// impl quat slerp
inline QuatF slerp(const QuatF& lhs, const QuatF& rhs, float t)
{
    return RtmConvert<QuatF>::from_rtm(
        rtm::quat_slerp(
            RtmConvert<QuatF>::to_rtm(lhs),
            RtmConvert<QuatF>::to_rtm(rhs),
            t
        )
    );
}
inline QuatD slerp(const QuatD& lhs, const QuatD& rhs, double t)
{
    return RtmConvert<QuatD>::from_rtm(
        rtm::quat_slerp(
            RtmConvert<QuatD>::to_rtm(lhs),
            RtmConvert<QuatD>::to_rtm(rhs),
            t
        )
    );
}

// log & exp
inline QuatF log(const QuatF& q)
{
    return RtmConvert<QuatF>::from_rtm(
        rtm::quat_rotation_log(
            RtmConvert<QuatF>::to_rtm(q)
        )
    );
}
inline QuatF exp(const QuatF& q)
{
    return RtmConvert<QuatF>::from_rtm(
        rtm::quat_rotation_exp(
            RtmConvert<QuatF>::to_rtm(q)
        )
    );
}

// is finite
inline bool is_finite(const QuatF& q)
{
    return rtm::quat_is_finite(
        RtmConvert<QuatF>::to_rtm(q)
    );
}
inline bool is_finite(const QuatD& q)
{
    return rtm::quat_is_finite(
        RtmConvert<QuatD>::to_rtm(q)
    );
}

// is normalized
inline bool is_normalized(const QuatF& q, float threshold = 0.00001f)
{
    return rtm::quat_is_normalized(
        RtmConvert<QuatF>::to_rtm(q),
        threshold
    );
}
inline bool is_normalized(const QuatD& q, double threshold = 0.00001)
{
    return rtm::quat_is_normalized(
        RtmConvert<QuatD>::to_rtm(q),
        threshold
    );
}

// equal
inline bool operator==(const QuatF& lhs, const QuatF& rhs)
{
    return rtm::quat_are_equal(
        RtmConvert<QuatF>::to_rtm(lhs),
        RtmConvert<QuatF>::to_rtm(rhs)
    );
}
inline bool operator==(const QuatD& lhs, const QuatD& rhs)
{
    return rtm::quat_are_equal(
        RtmConvert<QuatD>::to_rtm(lhs),
        RtmConvert<QuatD>::to_rtm(rhs)
    );
}

// not equal
inline bool operator!=(const QuatF& lhs, const QuatF& rhs)
{
    return !operator==(lhs, rhs);
}
inline bool operator!=(const QuatD& lhs, const QuatD& rhs)
{
    return !operator==(lhs, rhs);
}

// nearly equal
inline bool nearly_equal(const QuatF& lhs, const QuatF& rhs, float threshold = 0.00001f)
{
    return rtm::quat_near_equal(
        RtmConvert<QuatF>::to_rtm(lhs),
        RtmConvert<QuatF>::to_rtm(rhs),
        threshold
    );
}
inline bool nearly_equal(const QuatD& lhs, const QuatD& rhs, double threshold = 0.00001)
{
    return rtm::quat_near_equal(
        RtmConvert<QuatD>::to_rtm(lhs),
        RtmConvert<QuatD>::to_rtm(rhs),
        threshold
    );
}

// identity
inline bool is_identity(const QuatF& q)
{
    return q == QuatF::Identity();
}
inline bool is_identity(const QuatD& q)
{
    return q == QuatD::Identity();
}

// nearly identity
inline bool is_nearly_identity(const QuatF& q, float threshold_angle)
{
    return rtm::quat_near_identity(
        RtmConvert<QuatF>::to_rtm(q),
        threshold_angle
    );
}
inline bool is_nearly_identity(const QuatD& q, double threshold_angle)
{
    return rtm::quat_near_identity(
        RtmConvert<QuatD>::to_rtm(q),
        threshold_angle
    );
}
} // namespace math
} // namespace skr