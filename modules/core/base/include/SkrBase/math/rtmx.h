#pragma once
#include "rtm/matrix4x4f.h" // IWYU pragma: export
#include "rtm/mask4i.h" // IWYU pragma: export

// RH
namespace rtm
{
RTM_DISABLE_SECURITY_COOKIE_CHECK inline
uint32_t RTM_SIMD_CALL vector_to_snorm8(vector4f_arg0 v)
{
    const auto _x = rtm::vector_get_x(v);
    const auto _y = rtm::vector_get_y(v);
    const auto _z = rtm::vector_get_z(v);
    const auto _w = rtm::vector_get_w(v);

    const float scale = 127.0f / sqrtf(_x * _x + _y * _y + _z * _z);
    const uint32_t x = int(_x * scale);
    const uint32_t y = int(_y * scale);
    const uint32_t z = int(_z * scale);
    const uint32_t w = int(_w * scale);
    return (x & 0xff) | ((y & 0xff) << 8) | ((z & 0xff) << 16) | ((w & 0xff) << 24);
}
}
