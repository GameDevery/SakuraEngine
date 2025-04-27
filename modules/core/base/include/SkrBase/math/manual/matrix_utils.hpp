#pragma once
#include "./rtm_conv.hpp"
#include "../gen/misc/quat.hpp"
#include <rtm/camera_utilsf.h>
#include <rtm/camera_utilsd.h>

namespace skr
{
inline namespace math
{
// factory for utility usage
inline float4x4 float4x4::look_to(const float4& from, const float4& dir, const float4& up)
{
    return RtmConvert<float4x4>::from_rtm(
        rtm::matrix_cast(
            rtm::matrix_look_to(
                RtmConvert<float4>::to_rtm(from),
                RtmConvert<float4>::to_rtm(dir),
                RtmConvert<float4>::to_rtm(up)
            )
        )
    );
}
inline float4x4 float4x4::look_at(const float4& from, const float4& to, const float4& up)
{
    return RtmConvert<float4x4>::from_rtm(
        rtm::matrix_cast(
            rtm::matrix_look_at(
                RtmConvert<float4>::to_rtm(from),
                RtmConvert<float4>::to_rtm(to),
                RtmConvert<float4>::to_rtm(up)
            )
        )
    );
}
inline float4x4 float4x4::view_to(const float4& from, const float4& dir, const float4& up)
{
    return RtmConvert<float4x4>::from_rtm(
        rtm::matrix_cast(
            rtm::view_look_to(
                RtmConvert<float4>::to_rtm(from),
                RtmConvert<float4>::to_rtm(dir),
                RtmConvert<float4>::to_rtm(up)
            )
        )
    );
}
inline float4x4 float4x4::view_at(const float4& from, const float4& to, const float4& up)
{
    return RtmConvert<float4x4>::from_rtm(
        rtm::matrix_cast(
            rtm::view_look_at(
                RtmConvert<float4>::to_rtm(from),
                RtmConvert<float4>::to_rtm(to),
                RtmConvert<float4>::to_rtm(up)
            )
        )
    );
}
inline float4x4 float4x4::perspective(float view_width, float view_height, float near, float far)
{
    return RtmConvert<float4x4>::from_rtm(
        rtm::matrix_cast(
            rtm::proj_perspective(
                view_width,
                view_height,
                near,
                far
            )
        )
    );
}
inline float4x4 float4x4::perspective_fov(float fov_y, float aspect_ratio, float near, float far)
{
    return RtmConvert<float4x4>::from_rtm(
        rtm::matrix_cast(
            rtm::proj_perspective_fov(
                fov_y,
                aspect_ratio,
                near,
                far
            )
        )
    );
}
inline float4x4 float4x4::ortho(float width, float height, float near, float far)
{
    return RtmConvert<float4x4>::from_rtm(
        rtm::matrix_cast(
            rtm::proj_orthographic(
                width,
                height,
                near,
                far
            )
        )
    );
}
inline double4x4 double4x4::look_to(const double4& from, const double4& dir, const double4& up)
{
    return RtmConvert<double4x4>::from_rtm(
        rtm::matrix_cast(
            rtm::matrix_look_to(
                RtmConvert<double4>::to_rtm(from),
                RtmConvert<double4>::to_rtm(dir),
                RtmConvert<double4>::to_rtm(up)
            )
        )
    );
}
inline double4x4 double4x4::look_at(const double4& from, const double4& to, const double4& up)
{
    return RtmConvert<double4x4>::from_rtm(
        rtm::matrix_cast(
            rtm::matrix_look_at(
                RtmConvert<double4>::to_rtm(from),
                RtmConvert<double4>::to_rtm(to),
                RtmConvert<double4>::to_rtm(up)
            )
        )
    );
}
inline double4x4 double4x4::view_to(const double4& from, const double4& dir, const double4& up)
{
    return RtmConvert<double4x4>::from_rtm(
        rtm::matrix_cast(
            rtm::view_look_to(
                RtmConvert<double4>::to_rtm(from),
                RtmConvert<double4>::to_rtm(dir),
                RtmConvert<double4>::to_rtm(up)
            )
        )
    );
}
inline double4x4 double4x4::view_at(const double4& from, const double4& to, const double4& up)
{
    return RtmConvert<double4x4>::from_rtm(
        rtm::matrix_cast(
            rtm::view_look_at(
                RtmConvert<double4>::to_rtm(from),
                RtmConvert<double4>::to_rtm(to),
                RtmConvert<double4>::to_rtm(up)
            )
        )
    );
}
inline double4x4 double4x4::perspective(double view_width, double view_height, double near, double far)
{
    return RtmConvert<double4x4>::from_rtm(
        rtm::matrix_cast(
            rtm::proj_perspective(
                view_width,
                view_height,
                near,
                far
            )
        )
    );
}
inline double4x4 double4x4::perspective_fov(double fov_y, double aspect_ratio, double near, double far)
{
    return RtmConvert<double4x4>::from_rtm(
        rtm::matrix_cast(
            rtm::proj_perspective_fov(
                fov_y,
                aspect_ratio,
                near,
                far
            )
        )
    );
}
inline double4x4 double4x4::ortho(double width, double height, double near, double far)
{
    return RtmConvert<double4x4>::from_rtm(
        rtm::matrix_cast(
            rtm::proj_orthographic(
                width,
                height,
                near,
                far
            )
        )
    );
}

} // namespace math
} // namespace skr