#pragma once
#include "./rtm_conv.hpp"
#include "../gen/misc/quat.hpp"

namespace skr
{
inline namespace math
{
// convert with quat
inline RotatorF::RotatorF(const QuatF& quat)
{
    // get quaternion components
    float X = quat.x;
    float Y = quat.y;
    float Z = quat.z;
    float W = quat.w;

    const float SingularityTest = Z * X + W * Y;
    const float RollY           = 2.f * (W * Z - X * Y);
    const float RollX           = (1.f - 2.f * (Y * Y + Z * Z));

    // reference
    // http://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
    // http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToEuler/

    // this value was found from experience, the above websites recommend different values
    // but that isn't the case for us, so I went through different testing, and finally found the case
    // where both of world lives happily.
    const float SINGULARITY_THRESHOLD = 0.4999995f;
    if (SingularityTest < -SINGULARITY_THRESHOLD)
    {
        yaw   = -kPi / 2;
        roll  = (rtm::scalar_atan2(RollY, RollX));
        pitch = normalize_radians(roll + (2.f * rtm::scalar_atan2(X, W)));
    }
    else if (SingularityTest > SINGULARITY_THRESHOLD)
    {
        yaw   = kPi / 2;
        roll  = (rtm::scalar_atan2(RollY, RollX));
        pitch = normalize_radians(-roll + (2.f * rtm::scalar_atan2(X, W)));
    }
    else
    {
        yaw   = (rtm::scalar_asin(-2.f * SingularityTest));
        roll  = (rtm::scalar_atan2(RollY, RollX));
        pitch = (rtm::scalar_atan2(2.f * (W * X - Y * Z), (1.f - 2.f * (X * X + Y * Y))));
    }
}
} // namespace math
} // namespace skr