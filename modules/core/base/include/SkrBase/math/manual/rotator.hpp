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
} // namespace math
} // namespace skr