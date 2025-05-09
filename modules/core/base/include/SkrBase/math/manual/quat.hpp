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
    return QuatF{ RotatorF(pitch, yaw, roll) };
}
inline QuatD QuatD::Euler(double pitch, double yaw, double roll)
{
    return QuatD{ RotatorD(pitch, yaw, roll) };
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

// impl convert with rotator
inline QuatF::QuatF(const RotatorF& rotator)
{
    // we just need to match direction order, then the quat will be correct
    auto quat = rtm::quat_from_euler(
        rotator.yaw,  // [Right] rtm: pitch->y means [Right], skr: yaw->y means up
        rotator.roll, // [Up] rtm: yaw->z means [Up], skr: roll->z means forward
        rotator.pitch // [Forward] rtm: roll->x means [Forward], skr: pitch->x means right
    );
    RtmConvert<QuatF>::store(quat, *this);
}
inline QuatD::QuatD(const RotatorD& rotator)
{
    // we just need to match direction order, then the quat will be correct
    auto quat = rtm::quat_from_euler(
        rotator.yaw,  // [Right] rtm: pitch->y means [Right], skr: yaw->y means up
        rotator.roll, // [Up] rtm: yaw->z means [Up], skr: roll->z means forward
        rotator.pitch // [Forward] rtm: roll->x means [Forward], skr: pitch->x means right
    );
    RtmConvert<QuatD>::store(quat, *this);
}
inline QuatF QuatF::FromRotator(const RotatorF& rotator)
{
    return QuatF(rotator);
}
inline QuatD QuatD::FromRotator(const RotatorD& rotator)
{
    return QuatD(rotator);
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

// mul assign operator
inline QuatF& QuatF::operator*=(const QuatF& rhs)
{
    auto quat = rtm::quat_mul(
        RtmConvert<QuatF>::to_rtm(*this),
        RtmConvert<QuatF>::to_rtm(rhs)
    );
    RtmConvert<QuatF>::store(quat, *this);
    return *this;
}
inline QuatD& QuatD::operator*=(const QuatD& rhs)
{
    auto quat = rtm::quat_mul(
        RtmConvert<QuatD>::to_rtm(*this),
        RtmConvert<QuatD>::to_rtm(rhs)
    );
    RtmConvert<QuatD>::store(quat, *this);
    return *this;
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
inline bool is_normalized(const QuatF& q, float threshold = 1.e-4)
{
    return rtm::quat_is_normalized(
        RtmConvert<QuatF>::to_rtm(q),
        threshold
    );
}
inline bool is_normalized(const QuatD& q, double threshold = 1.e-4)
{
    return rtm::quat_is_normalized(
        RtmConvert<QuatD>::to_rtm(q),
        threshold
    );
}

// nearly equal
inline bool nearly_equal(const QuatF& lhs, const QuatF& rhs, float threshold = 1.e-4f)
{
    return rtm::quat_near_equal(
        RtmConvert<QuatF>::to_rtm(lhs),
        RtmConvert<QuatF>::to_rtm(rhs),
        threshold
    );
}
inline bool nearly_equal(const QuatD& lhs, const QuatD& rhs, double threshold = 1.e-4)
{
    return rtm::quat_near_equal(
        RtmConvert<QuatD>::to_rtm(lhs),
        RtmConvert<QuatD>::to_rtm(rhs),
        threshold
    );
}

// identity
inline bool QuatF::is_identity() const
{
    return all(as_vector() == QuatF::Identity().as_vector());
}
inline bool QuatD::is_identity() const
{
    return all(as_vector() == QuatD::Identity().as_vector());
}

// nearly identity
inline bool QuatF::is_nearly_identity(float threshold_angle) const
{
    return rtm::quat_near_identity(
        RtmConvert<QuatF>::to_rtm(*this),
        threshold_angle
    );
}
inline bool QuatD::is_nearly_identity(double threshold_angle) const
{
    return rtm::quat_near_identity(
        RtmConvert<QuatD>::to_rtm(*this),
        threshold_angle
    );
}

// to matrix3
inline QuatF::operator float3x3() const
{
    return to_matrix3();
}
inline float3x3 QuatF::to_matrix3() const
{
    return RtmConvert<float3x3>::from_rtm(
        rtm::matrix_cast(
            (rtm::matrix3x4f)rtm::matrix_from_quat(
                RtmConvert<QuatF>::to_rtm(*this)
            )
        )
    );
}
inline QuatD::operator double3x3() const
{
    return to_matrix3();
}
inline double3x3 QuatD::to_matrix3() const
{
    return RtmConvert<double3x3>::from_rtm(
        rtm::matrix_cast(
            (rtm::matrix3x4d)rtm::matrix_from_quat(
                RtmConvert<QuatD>::to_rtm(*this)
            )
        )
    );
}

// to matrix4
inline QuatF::operator float4x4() const
{
    return to_matrix4();
}
inline float4x4 QuatF::to_matrix4() const
{
    return RtmConvert<float4x4>::from_rtm(
        rtm::matrix_cast(
            (rtm::matrix3x4f)rtm::matrix_from_quat(
                RtmConvert<QuatF>::to_rtm(*this)
            )
        )
    );
}
inline QuatD::operator double4x4() const
{
    return to_matrix4();
}
inline double4x4 QuatD::to_matrix4() const
{
    return RtmConvert<double4x4>::from_rtm(
        rtm::matrix_cast(
            (rtm::matrix3x4d)rtm::matrix_from_quat(
                RtmConvert<QuatD>::to_rtm(*this)
            )
        )
    );
}

// from matrix ctor
inline QuatF::QuatF(const float3x3& mat)
{
    auto quat = rtm::quat_from_matrix(
        RtmConvert<float3x3>::to_rtm(mat)
    );
    RtmConvert<QuatF>::store(quat, *this);
}
inline QuatF::QuatF(const float4x4& mat)
{
    auto quat = rtm::quat_from_matrix(
        (rtm::matrix3x4f)rtm::matrix_cast(
            RtmConvert<float4x4>::to_rtm(mat)
        )
    );
    RtmConvert<QuatF>::store(quat, *this);
}
inline QuatD::QuatD(const double3x3& mat)
{
    auto quat = rtm::quat_from_matrix(
        RtmConvert<double3x3>::to_rtm(mat)
    );
    RtmConvert<QuatD>::store(quat, *this);
}
inline QuatD::QuatD(const double4x4& mat)
{
    auto quat = rtm::quat_from_matrix(
        (rtm::matrix3x4d)rtm::matrix_cast(
            RtmConvert<double4x4>::to_rtm(mat)
        )
    );
    RtmConvert<QuatD>::store(quat, *this);
}

// from matrix factory
inline QuatF QuatF::FromMatrix(const float3x3& mat)
{
    return QuatF(mat);
}
inline QuatF QuatF::FromMatrix(const float4x4& mat)
{
    return QuatF(mat);
}
inline QuatD QuatD::FromMatrix(const double3x3& mat)
{
    return QuatD(mat);
}
inline QuatD QuatD::FromMatrix(const double4x4& mat)
{
    return QuatD(mat);
}

// compare
inline bool4 operator==(const QuatF& lhs, const QuatF& rhs) SKR_NOEXCEPT
{
    return lhs.as_vector() == rhs.as_vector();
}
inline bool4 operator==(const QuatD& lhs, const QuatD& rhs) SKR_NOEXCEPT
{
    return lhs.as_vector() == rhs.as_vector();
}
inline bool4 operator!=(const QuatF& lhs, const QuatF& rhs) SKR_NOEXCEPT
{
    return lhs.as_vector() != rhs.as_vector();
}
inline bool4 operator!=(const QuatD& lhs, const QuatD& rhs) SKR_NOEXCEPT
{
    return lhs.as_vector() != rhs.as_vector();
}

// distance
inline float distance(const QuatF& lhs, const QuatF& rhs)
{
    return acos(2 * dot(lhs, rhs) - 1);
}
inline double distance(const QuatD& lhs, const QuatD& rhs)
{
    return acos(2 * dot(lhs, rhs) - 1);
}

// relative
inline QuatF relative(const QuatF& parent, const QuatF& world)
{
    auto rtm_parent = RtmConvert<QuatF>::to_rtm(parent);
    auto rtm_world  = RtmConvert<QuatF>::to_rtm(world);

    // world * -parent
    auto rtm_result = rtm::quat_mul(
        rtm_world,
        rtm::quat_conjugate(rtm_parent)
    );

    return RtmConvert<QuatF>::from_rtm(rtm_result);
}
inline QuatD relative(const QuatD& parent, const QuatD& world)
{
    auto rtm_parent = RtmConvert<QuatD>::to_rtm(parent);
    auto rtm_world  = RtmConvert<QuatD>::to_rtm(world);

    // world * -parent
    auto result_rtm = rtm::quat_mul(
        rtm_world,
        rtm::quat_conjugate(rtm_parent)
    );

    return RtmConvert<QuatD>::from_rtm(result_rtm);
}
} // namespace math
} // namespace skr