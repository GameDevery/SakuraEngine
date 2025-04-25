#pragma once
#include "../gen/misc/quat.hpp"
#include "rtm/quatf.h"
#include "rtm/quatd.h"

namespace skr
{
// rtm caster
inline namespace math
{
inline rtm::quatf to_rtm(const QuatF& q)
{
    return rtm::quat_load(&q.x);
}
inline QuatF from_rtm(const rtm::quatf& q)
{
    QuatF result;
    rtm::quat_store(q, &result.x);
    return result;
}
inline rtm::quatd to_rtm(const QuatD& q)
{
    return rtm::quat_load(&q.x);
}
inline QuatD from_rtm(const rtm::quatd& q)
{
    QuatD result;
    rtm::quat_store(q, &result.x);
    return result;
}
} // namespace math

} // namespace skr