#pragma once
#include "./rtm_conv.hpp"
#include "../gen/misc/quat.hpp"

namespace skr
{
inline namespace math
{
// shift
inline void camera_apply_shift(float4x4& matrix, const float2& shift)
{
    matrix.forward.x += shift.x;
    matrix.forward.y += shift.y;
}
inline void camera_apply_shift(double4x4& matrix, const double2& shift)
{
    matrix.forward.x += shift.x;
    matrix.forward.y += shift.y;
}

// offset
inline void camera_apply_offset(float4x4& matrix, const float2& offset)
{
    matrix.translation.x += offset.x;
    matrix.translation.y += offset.y;
}
inline void camera_apply_offset(double4x4& matrix, const double2& offset)
{
    matrix.translation.x += offset.x;
    matrix.translation.y += offset.y;
}

// apply scale
inline void camera_apply_scale(float4x4& matrix, const float2& scale)
{
    matrix.rows[0][0] *= scale.x;
    matrix.rows[1][1] *= scale.y;
}
inline void camera_apply_scale(double4x4& matrix, const double2& scale)
{
    matrix.rows[0][0] *= scale.x;
    matrix.rows[1][1] *= scale.y;
}

// fov x from y and aspect ratio
inline float camera_fov_x_from_y(float fov_y, float aspect_ratio)
{
    return 2.0f * atan(tan(fov_y * 0.5f) * aspect_ratio);
}
inline double camera_fov_x_from_y(double fov_y, double aspect_ratio)
{
    return 2.0 * atan(tan(fov_y * 0.5) * aspect_ratio);
}

// fov y from x and aspect ratio
inline float camera_fov_y_from_x(float fov_x, float aspect_ratio)
{
    return 2.0f * atan(tan(fov_x * 0.5f) / aspect_ratio);
}
inline double camera_fov_y_from_x(double fov_x, double aspect_ratio)
{
    return 2.0 * atan(tan(fov_x * 0.5) / aspect_ratio);
}

// aspect ratio syntax
inline float camera_aspect_ratio(float width, float height)
{
    return width / height;
}

// rotation to shift
inline float camera_rotation_to_shift(float rotation, float fov)
{
    float factor = (kPi / 4) / (fov / 2);
    return tan(rotation * factor);
}
inline double camera_rotation_to_shift(double rotation, double fov)
{
    double factor = (kPi / 4) / (fov / 2);
    return tan(rotation * factor);
}

// shift to rotation
inline float camera_shift_to_rotation(float shift, float fov)
{
    float factor = (kPi / 4) / (fov / 2);
    return atan(shift) / factor;
}
inline double camera_shift_to_rotation(double shift, double fov)
{
    double factor = (kPi / 4) / (fov / 2);
    return atan(shift) / factor;
}

} // namespace math
} // namespace skr