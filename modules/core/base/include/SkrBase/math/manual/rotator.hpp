#pragma once
#include "./rtm_conv.hpp"
#include "../gen/misc/quat.hpp"

namespace skr
{
inline namespace math
{
template <typename Real, typename Rotator, typename Quat>
inline void _convert_quat_to_rotator(const Quat& quat, Rotator& rotator)
{
    // get quaternion components
    Real X = quat.x;
    Real Y = quat.y;
    Real Z = quat.z;
    Real W = quat.w;

    const Real SingularityTest = Z * X + W * Y;
    const Real RollY           = 2.f * (W * Z - X * Y);
    const Real RollX           = (1.f - 2.f * (Y * Y + Z * Z));

    // reference
    // http://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
    // http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToEuler/

    // this value was found from experience, the above websites recommend different values
    // but that isn't the case for us, so I went through different testing, and finally found the case
    // where both of world lives happily.
    const Real SINGULARITY_THRESHOLD = 0.4999995f;
    if (SingularityTest < -SINGULARITY_THRESHOLD)
    {
        rotator.yaw   = -kPi / 2;
        rotator.roll  = (rtm::scalar_atan2(RollY, RollX));
        rotator.pitch = normalize_radians(rotator.roll + (2.f * rtm::scalar_atan2(X, W)));
    }
    else if (SingularityTest > SINGULARITY_THRESHOLD)
    {
        rotator.yaw   = kPi / 2;
        rotator.roll  = (rtm::scalar_atan2(RollY, RollX));
        rotator.pitch = normalize_radians(-rotator.roll + (2.f * rtm::scalar_atan2(X, W)));
    }
    else
    {
        rotator.yaw   = (rtm::scalar_asin(-2.f * SingularityTest));
        rotator.roll  = (rtm::scalar_atan2(RollY, RollX));
        rotator.pitch = (rtm::scalar_atan2(2.f * (W * X - Y * Z), (1.f - 2.f * (X * X + Y * Y))));
    }
}

// convert with quat
inline RotatorF::RotatorF(const QuatF& quat)
{
    _convert_quat_to_rotator<float, RotatorF, QuatF>(quat, *this);
}
inline RotatorD::RotatorD(const QuatD& quat)
{
    _convert_quat_to_rotator<double, RotatorD, QuatD>(quat, *this);
}
inline RotatorF RotatorF::FromQuat(const QuatF& quat)
{
    return RotatorF(quat);
}
inline RotatorD RotatorD::FromQuat(const QuatD& quat)
{
    return RotatorD(quat);
}

// compare
inline bool3 operator==(const RotatorF& lhs, const RotatorF& rhs) SKR_NOEXCEPT
{
    return lhs.as_vector() == rhs.as_vector();
}
inline bool3 operator==(const RotatorD& lhs, const RotatorD& rhs) SKR_NOEXCEPT
{
    return lhs.as_vector() == rhs.as_vector();
}
inline bool3 operator!=(const RotatorF& lhs, const RotatorF& rhs) SKR_NOEXCEPT
{
    return lhs.as_vector() != rhs.as_vector();
}
inline bool3 operator!=(const RotatorD& lhs, const RotatorD& rhs) SKR_NOEXCEPT
{
    return lhs.as_vector() != rhs.as_vector();
}

// negative operator
inline RotatorF RotatorF::operator-() const
{
    return { -pitch, -yaw, -roll };
}
inline RotatorD RotatorD::operator-() const
{
    return { -pitch, -yaw, -roll };
}

// mul assign operator
inline RotatorF& RotatorF::operator*=(const RotatorF& rhs)
{
    auto quat = QuatF(*this) * QuatF(rhs);
    _convert_quat_to_rotator<float, RotatorF, QuatF>(quat, *this);
    return *this;
}
inline RotatorD& RotatorD::operator*=(const RotatorD& rhs)
{
    auto quat = QuatD(*this) * QuatD(rhs);
    _convert_quat_to_rotator<double, RotatorD, QuatD>(quat, *this);
    return *this;
}

// add & sub assign operator
inline RotatorF& RotatorF::operator+=(const RotatorF& rhs)
{
    pitch += rhs.pitch;
    yaw += rhs.yaw;
    roll += rhs.roll;
    return *this;
}
inline RotatorF& RotatorF::operator-=(const RotatorF& rhs)
{
    pitch -= rhs.pitch;
    yaw -= rhs.yaw;
    roll -= rhs.roll;
    return *this;
}
inline RotatorD& RotatorD::operator+=(const RotatorD& rhs)
{
    pitch += rhs.pitch;
    yaw += rhs.yaw;
    roll += rhs.roll;
    return *this;
}
inline RotatorD& RotatorD::operator-=(const RotatorD& rhs)
{
    pitch -= rhs.pitch;
    yaw -= rhs.yaw;
    roll -= rhs.roll;
    return *this;
}

// to matrix operator
inline RotatorF::operator float3x3() const
{
    return QuatF(*this).to_matrix3();
}
inline RotatorF::operator float4x4() const
{
    return QuatF(*this).to_matrix4();
}
inline RotatorD::operator double3x3() const
{
    return QuatD(*this).to_matrix3();
}
inline RotatorD::operator double4x4() const
{
    return QuatD(*this).to_matrix4();
}

// to matrix function
inline float3x3 RotatorF::to_matrix3() const
{
    return QuatF(*this).to_matrix3();
}
inline float4x4 RotatorF::to_matrix4() const
{
    return QuatF(*this).to_matrix4();
}
inline double3x3 RotatorD::to_matrix3() const
{
    return QuatD(*this).to_matrix3();
}
inline double4x4 RotatorD::to_matrix4() const
{
    return QuatD(*this).to_matrix4();
}

// from matrix ctor
inline RotatorF::RotatorF(const float3x3& mat)
{
    *this = RotatorF(QuatF::FromMatrix(mat));
}
inline RotatorF::RotatorF(const float4x4& mat)
{
    *this = RotatorF(QuatF::FromMatrix(mat));
}
inline RotatorD::RotatorD(const double3x3& mat)
{
    *this = RotatorD(QuatD::FromMatrix(mat));
}
inline RotatorD::RotatorD(const double4x4& mat)
{
    *this = RotatorD(QuatD::FromMatrix(mat));
}

// from matrix function
inline RotatorF RotatorF::FromMatrix(const float3x3& mat)
{
    return RotatorF(QuatF::FromMatrix(mat));
}
inline RotatorF RotatorF::FromMatrix(const float4x4& mat)
{
    return RotatorF(QuatF::FromMatrix(mat));
}
inline RotatorD RotatorD::FromMatrix(const double3x3& mat)
{
    return RotatorD(QuatD::FromMatrix(mat));
}
inline RotatorD RotatorD::FromMatrix(const double4x4& mat)
{
    return RotatorD(QuatD::FromMatrix(mat));
}

// mul
inline RotatorF mul(const RotatorF& lhs, const RotatorF& rhs)
{
    return RotatorF(QuatF(lhs) * QuatF(rhs));
}
inline RotatorD mul(const RotatorD& lhs, const RotatorD& rhs)
{
    return RotatorD(QuatD(lhs) * QuatD(rhs));
}
inline RotatorF operator*(const RotatorF& lhs, const RotatorF& rhs)
{
    return mul(lhs, rhs);
}
inline RotatorD operator*(const RotatorD& lhs, const RotatorD& rhs)
{
    return mul(lhs, rhs);
}

// add
inline RotatorF operator+(const RotatorF& lhs, const RotatorF& rhs)
{
    return {
        lhs.pitch + rhs.pitch,
        lhs.yaw + rhs.yaw,
        lhs.roll + rhs.roll,
    };
}
inline RotatorF operator-(const RotatorF& lhs, const RotatorF& rhs)
{
    return {
        lhs.pitch - rhs.pitch,
        lhs.yaw - rhs.yaw,
        lhs.roll - rhs.roll,
    };
}
inline RotatorD operator+(const RotatorD& lhs, const RotatorD& rhs)
{
    return {
        lhs.pitch + rhs.pitch,
        lhs.yaw + rhs.yaw,
        lhs.roll + rhs.roll,
    };
}
inline RotatorD operator-(const RotatorD& lhs, const RotatorD& rhs)
{
    return {
        lhs.pitch - rhs.pitch,
        lhs.yaw - rhs.yaw,
        lhs.roll - rhs.roll,
    };
}

// vector mul
inline float3 operator*(const float3& lhs, const RotatorF& rhs)
{
    return mul(lhs, QuatF(rhs));
}
inline double3 operator*(const double3& lhs, const RotatorD& rhs)
{
    return mul(lhs, QuatD(rhs));
}
inline float3 mul(const float3& lhs, const RotatorF& rhs)
{
    return mul(lhs, QuatF(rhs));
}
inline double3 mul(const double3& lhs, const RotatorD& rhs)
{
    return mul(lhs, QuatD(rhs));
}

} // namespace math
} // namespace skr