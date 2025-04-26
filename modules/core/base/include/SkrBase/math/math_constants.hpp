#pragma once

namespace skr
{
inline namespace math
{
inline constexpr float kPi  = 3.14159265358979323846f;
inline constexpr float kPi2 = 6.28318530717958647692f; // 2 * pi

enum class EAxis3
{
    X = 0,
    Y = 1,
    Z = 2,
};
enum class EAxis4
{
    X = 0,
    Y = 1,
    Z = 2,
    W = 3,
};

} // namespace math
} // namespace skr