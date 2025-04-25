#pragma once
#include "../gen/misc/quat.hpp"
#include "rtm/quatf.h"
#include "rtm/quatd.h"

namespace skr
{
// rtm caster
inline namespace math
{
inline rtm::quatf to_rtm(const Quatf& q)
{
    return rtm::quat_load(&q.x);
}
inline Quatf from_rtm(const rtm::quatf& q)
{
    Quatf result;
    rtm::quat_store(q, &result.x);
    return result;
}
inline rtm::quatd to_rtm(const Quatd& q)
{
    return rtm::quat_load(&q.x);
}
inline Quatd from_rtm(const rtm::quatd& q)
{
    Quatd result;
    rtm::quat_store(q, &result.x);
    return result;
}
} // namespace math

} // namespace skr