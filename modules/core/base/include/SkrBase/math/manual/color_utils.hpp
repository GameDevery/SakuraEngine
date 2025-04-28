#pragma once
#include "./rtm_conv.hpp"
#include "../gen/misc/quat.hpp"

namespace skr
{
inline namespace math
{
enum class EWhitePoint : uint8_t
{
    CIE1931_D65,
    ACES_D60,
    DCI_CalibrationWhite,
};
enum class EChromaticAdaptationMethod : uint8_t
{
    XYZScaling,
    Bradford,
    CAT02,
    VonKries,
};
enum class EColorSpace : uint8_t
{
    sRGB,
    Rec2020,
    ACES_AP0,
    ACES_AP1,
    P3DCI,
    P3D65,
    REDWideGamut,
    SonySGamut3,
    SonySGamut3Cine,
    AlexaWideGamut,
    CanonCinemaGamut,
    GoProProtuneNative,
    PanasonicVGamut,
    PLASA_E1_54,
};

// TODO. color encoding TransferFunctions.h

// chromaticities in xy coordinate
struct Chromaticities {
    Chromaticities(EColorSpace color_space);
    Chromaticities(
        const double2& red,
        const double2& green,
        const double2& blue,
        const double2& white_point
    );

    // xyY <=> XYZ
    static double3 xyY_to_XYZ(const double3& xyY);
    static double3 XYZ_to_xyY(const double3& XYZ);

    // xyY <=> LMS
    static double3 XYZ_to_LMS(const double3& XYZ, EChromaticAdaptationMethod method = EChromaticAdaptationMethod::Bradford);
    static double3 LMS_to_XYZ(const double3& LMS, EChromaticAdaptationMethod method = EChromaticAdaptationMethod::Bradford);

    // white point getter
    static double2 get_white_point(EWhitePoint white_point);

    // chromatic adaptation matrix
    // see: http://brucelindbloom.com/index.html?Eqn_ChromAdapt.html
    static double3x3 chromatic_adaptation_matrix(EChromaticAdaptationMethod method);
    static double3x3 chromatic_adaptation_matrix(
        EChromaticAdaptationMethod method,
        const double3&             from_white_in_XYZ,
        const double3&             to_white_in_XYZ
    );

    // Accurate for 1000K < Temp < 15000K
    // [Krystek 1985, "An algorithm to calculate correlated colour temperature"]
    static double2 cct_to_xy_planckian_locus(double cct);

    // Accurate for 4000K < Temp < 25000K
    // in: correlated color temperature
    // out: CIE 1931 chromaticity
    static double2 cct_to_xy_daylight(double cct);

    // cct/duv to xy
    static double2 cct_duv_to_xy(
        double cct,
        double duv
    );

    // white balance matrix
    static double3x3 white_balance_matrix(
        double2                    reference_white,
        double2                    target_white_xy,
        bool                       is_white_balance_mode,
        EChromaticAdaptationMethod method = EChromaticAdaptationMethod::Bradford
    );

    // get planckian isothermal offset
    static float2 planckian_isothermal(float cct, float duv);

    // setup by color space
    void setup_by_color_space(EColorSpace color_space);

    // calc to_xyz matrix
    double3x3 matrix_to_xyz();

    double2 red;
    double2 green;
    double2 blue;
    double2 white_point;
};

// color space
struct ColorSpace {
    ColorSpace(EColorSpace color_space);
    ColorSpace(
        const double2& red,
        const double2& green,
        const double2& blue,
        const double2& white_point
    );

    // convert matrix
    static double3x3 convert_matrix(
        const ColorSpace&          from,
        const ColorSpace&          to,
        EChromaticAdaptationMethod method = EChromaticAdaptationMethod::Bradford
    );

    // convert color
    double3 color_to_XYZ(const double3& color) const;
    double3 color_from_XYZ(const double3& XYZ) const;

    Chromaticities chromaticities;
    double3x3      to_xyz;
    double3x3      from_xyz;
};

} // namespace math
} // namespace skr

namespace skr
{
inline namespace math
{
inline Chromaticities::Chromaticities(EColorSpace color_space)
    : red(kMathNoInit)
    , green(kMathNoInit)
    , blue(kMathNoInit)
    , white_point(kMathNoInit)
{
    setup_by_color_space(color_space);
}
inline Chromaticities::Chromaticities(
    const double2& red,
    const double2& green,
    const double2& blue,
    const double2& white_point
)
    : red(red)
    , green(green)
    , blue(blue)
    , white_point(white_point)
{
}

// xyY <=> XYZ
inline double3 Chromaticities::xyY_to_XYZ(const double3& xyY)
{
    double divisor = max(xyY.y, 1e-10);

    return {
        xyY.x * xyY.z / divisor,
        xyY.z,
        (1.0 - xyY.x - xyY.y) * xyY.z / divisor
    };
}
inline double3 Chromaticities::XYZ_to_xyY(const double3& XYZ)
{
    double divisor = XYZ.x + XYZ.y + XYZ.z;
    if (divisor == 0.0)
    {
        divisor = 1e-10;
    }

    return {
        XYZ.x / divisor,
        XYZ.y / divisor,
        XYZ.y
    };
}

// xyY <=> LMS
inline double3 Chromaticities::XYZ_to_LMS(const double3& XYZ, EChromaticAdaptationMethod method)
{
    return XYZ * chromatic_adaptation_matrix(method);
}
inline double3 Chromaticities::LMS_to_XYZ(const double3& LMS, EChromaticAdaptationMethod method)
{
    return LMS * inverse(chromatic_adaptation_matrix(method));
}

inline double2 Chromaticities::get_white_point(EWhitePoint white_point)
{
    switch (white_point)
    {
    case EWhitePoint::CIE1931_D65:
        return { 0.3127, 0.3290 };
    case EWhitePoint::ACES_D60:
        return { 0.32168, 0.33767 };
    case EWhitePoint::DCI_CalibrationWhite:
        return { 0.314, 0.351 };
    default:
        SKR_UNREACHABLE_CODE()
        return {};
    }
}

inline double3x3 Chromaticities::chromatic_adaptation_matrix(EChromaticAdaptationMethod method)
{
    switch (method)
    {
    case EChromaticAdaptationMethod::XYZScaling:
        return double3x3::identity();
    case EChromaticAdaptationMethod::Bradford:
        return transpose(double3x3{
            { 0.8951, 0.2664, -0.1614 },
            { -0.7502, 1.7135, 0.0367 },
            { 0.0389, -0.0685, 1.0296 },
        });
    case EChromaticAdaptationMethod::CAT02:
        return transpose(double3x3{
            { 0.7328, 0.4296, -0.1624 },
            { -0.7036, 1.6975, 0.0061 },
            { 0.0030, 0.0136, 0.9834 },
        });
    case EChromaticAdaptationMethod::VonKries:
        return transpose(double3x3{
            { 0.4002, 0.7076, -0.0808 },
            { -0.2263, 1.1653, 0.0457 },
            { 0.0000, 0.0000, 0.8252 },
        });
    default:
        SKR_UNREACHABLE_CODE();
        return double3x3::identity();
    }
}
inline double3x3 Chromaticities::chromatic_adaptation_matrix(
    EChromaticAdaptationMethod method,
    const double3&             from_white_in_XYZ,
    const double3&             to_white_in_XYZ
)
{
    double3x3 to_cone_response = chromatic_adaptation_matrix(method);

    auto from_white_cone_response = from_white_in_XYZ * to_cone_response;
    auto to_white_cone_response   = to_white_in_XYZ * to_cone_response;

    auto scale = to_white_cone_response / from_white_cone_response;

    auto scale_matrix = double3x3::from_scale(scale);

    return to_cone_response * scale_matrix * inverse(to_cone_response);
}

// calc xy from temperature
inline double2 Chromaticities::cct_to_xy_planckian_locus(double cct)
{
    cct = clamp(cct, 1000.0, 15000.0);

    // Approximate Planckian locus in CIE 1960 UCS
    float u = (0.860117757f + 1.54118254e-4f * cct + 1.28641212e-7f * cct * cct) / (1.0f + 8.42420235e-4f * cct + 7.08145163e-7f * cct * cct);
    float v = (0.317398726f + 4.22806245e-5f * cct + 4.20481691e-8f * cct * cct) / (1.0f - 2.89741816e-5f * cct + 1.61456053e-7f * cct * cct);

    float x = 3.0f * u / (2.0f * u - 8.0f * v + 4.0f);
    float y = 2.0f * v / (2.0f * u - 8.0f * v + 4.0f);

    return { x, y };
}
inline double2 Chromaticities::cct_to_xy_daylight(double cct)
{
    // Correct for revision of Plank's law
    // This makes 6500 == D65
    cct *= 1.4388 / 1.438;
    float rcp_cct = 1.0 / cct;
    float x       = cct <= 7000 ?
                        0.244063 + (0.09911e3 + (2.9678e6 - 4.6070e9 * rcp_cct) * rcp_cct) * rcp_cct :
                        0.237040 + (0.24748e3 + (1.9018e6 - 2.0064e9 * rcp_cct) * rcp_cct) * rcp_cct;

    float y = -3 * x * x + 2.87 * x - 0.275;

    return float2(x, y);
}
inline float2 Chromaticities::planckian_isothermal(float cct, float duv)
{
    float u = (0.860117757f + 1.54118254e-4f * cct + 1.28641212e-7f * cct * cct) / (1.0f + 8.42420235e-4f * cct + 7.08145163e-7f * cct * cct);
    float v = (0.317398726f + 4.22806245e-5f * cct + 4.20481691e-8f * cct * cct) / (1.0f - 2.89741816e-5f * cct + 1.61456053e-7f * cct * cct);

    float ud = (-1.13758118e9f - 1.91615621e6f * cct - 1.53177f * cct * cct) / square(1.41213984e6f + 1189.62f * cct + cct * cct);
    float vd = (1.97471536e9f - 705674.0f * cct - 308.607f * cct * cct) / square(6.19363586e6f - 179.456f * cct + cct * cct);

    float2 uvd = normalize(float2(ud, vd));

    // Correlated color temperature is meaningful within +/- 0.05
    u += uvd.y * duv * 0.05;
    v += -uvd.x * duv * 0.05;

    float x = 3 * u / (2 * u - 8 * v + 4);
    float y = 2 * v / (2 * u - 8 * v + 4);

    return float2(x, y);
}
inline double2 Chromaticities::cct_duv_to_xy(
    double cct,
    double duv
)
{
    double2 white_point_daylight        = cct_to_xy_daylight(cct);
    double2 white_point_planckian       = cct_to_xy_planckian_locus(cct);
    double2 planckian_isothermal_offset = planckian_isothermal(cct, duv) - white_point_planckian;

    if (cct < 4000)
    {
        return white_point_planckian + planckian_isothermal_offset;
    }
    else
    {
        return white_point_daylight + planckian_isothermal_offset;
    }
}
inline double3x3 Chromaticities::white_balance_matrix(
    double2                    reference_white,
    double2                    target_white_xy,
    bool                       is_white_balance_mode,
    EChromaticAdaptationMethod method
)
{
    if (is_white_balance_mode)
    {
        return chromatic_adaptation_matrix(
            method,
            xyY_to_XYZ({ target_white_xy, 1.0 }),
            xyY_to_XYZ({ reference_white, 1.0 })
        );
    }
    else
    {
        return chromatic_adaptation_matrix(
            method,
            xyY_to_XYZ({ reference_white, 1.0 }),
            xyY_to_XYZ({ target_white_xy, 1.0 })
        );
    }
}

inline void Chromaticities::setup_by_color_space(EColorSpace color_space)
{
    switch (color_space)
    {
    case EColorSpace::sRGB:
        red         = { 0.64, 0.33 };
        green       = { 0.30, 0.60 };
        blue        = { 0.15, 0.06 };
        white_point = get_white_point(EWhitePoint::CIE1931_D65);
        break;
    case EColorSpace::Rec2020:
        red         = { 0.708, 0.292 };
        green       = { 0.170, 0.797 };
        blue        = { 0.131, 0.046 };
        white_point = get_white_point(EWhitePoint::CIE1931_D65);
        break;
    case EColorSpace::ACES_AP0:
        red         = { 0.7347, 0.2653 };
        green       = { 0.0000, 1.0000 };
        blue        = { 0.0001, -0.0770 };
        white_point = get_white_point(EWhitePoint::ACES_D60);
        break;
    case EColorSpace::ACES_AP1:
        red         = { 0.713, 0.293 };
        green       = { 0.165, 0.830 };
        blue        = { 0.128, 0.044 };
        white_point = get_white_point(EWhitePoint::ACES_D60);
        break;
    case EColorSpace::P3DCI:
        red         = { 0.680, 0.320 };
        green       = { 0.265, 0.690 };
        blue        = { 0.150, 0.060 };
        white_point = get_white_point(EWhitePoint::DCI_CalibrationWhite);
        break;
    case EColorSpace::P3D65:
        red         = { 0.680, 0.320 };
        green       = { 0.265, 0.690 };
        blue        = { 0.150, 0.060 };
        white_point = get_white_point(EWhitePoint::CIE1931_D65);
        break;
    case EColorSpace::REDWideGamut:
        red         = { 0.780308, 0.304253 };
        green       = { 0.121595, 1.493994 };
        blue        = { 0.095612, -0.084589 };
        white_point = get_white_point(EWhitePoint::CIE1931_D65);
        break;
    case EColorSpace::SonySGamut3:
        red         = { 0.730, 0.280 };
        green       = { 0.140, 0.855 };
        blue        = { 0.100, -0.050 };
        white_point = get_white_point(EWhitePoint::CIE1931_D65);
        break;
    case EColorSpace::SonySGamut3Cine:
        red         = { 0.766, 0.275 };
        green       = { 0.225, 0.800 };
        blue        = { 0.089, -0.087 };
        white_point = get_white_point(EWhitePoint::CIE1931_D65);
        break;
    case EColorSpace::AlexaWideGamut:
        red         = { 0.684, 0.313 };
        green       = { 0.221, 0.848 };
        blue        = { 0.0861, -0.1020 };
        white_point = get_white_point(EWhitePoint::CIE1931_D65);
        break;
    case EColorSpace::CanonCinemaGamut:
        red         = { 0.740, 0.270 };
        green       = { 0.170, 1.140 };
        blue        = { 0.080, -0.100 };
        white_point = get_white_point(EWhitePoint::CIE1931_D65);
        break;
    case EColorSpace::GoProProtuneNative:
        red         = { 0.698448, 0.193026 };
        green       = { 0.329555, 1.024597 };
        blue        = { 0.108443, -0.034679 };
        white_point = get_white_point(EWhitePoint::CIE1931_D65);
        break;
    case EColorSpace::PanasonicVGamut:
        red         = { 0.730, 0.280 };
        green       = { 0.165, 0.840 };
        blue        = { 0.100, -0.030 };
        white_point = get_white_point(EWhitePoint::CIE1931_D65);
        break;
    case EColorSpace::PLASA_E1_54:
        red         = { 0.7347, 0.2653 };
        green       = { 0.1596, 0.8404 };
        blue        = { 0.0366, 0.0001 };
        white_point = { 0.4254, 0.4044 };
        break;
    default:
        SKR_UNREACHABLE_CODE();
        break;
    }
}

inline double3x3 Chromaticities::matrix_to_xyz()
{
    double3x3 mat = inverse(double3x3{
        { red.x, red.y, 1 - red.x - red.y },
        { green.x, green.y, 1 - green.x - green.y },
        { blue.x, blue.y, 1 - blue.x - blue.y },
    });

    double3 white_xyz = xyY_to_XYZ({ white_point, 1.0 });

    double3 scale = mul(white_xyz, mat);

    mat.axis_x *= scale.x;
    mat.axis_y *= scale.y;
    mat.axis_z *= scale.z;

    return mat;
}

inline ColorSpace::ColorSpace(EColorSpace color_space)
    : chromaticities(color_space)
    , to_xyz(kMathNoInit)
    , from_xyz(kMathNoInit)
{
    to_xyz   = chromaticities.matrix_to_xyz();
    from_xyz = inverse(to_xyz);
}
inline ColorSpace::ColorSpace(
    const double2& red,
    const double2& green,
    const double2& blue,
    const double2& white_point
)
    : chromaticities(red, green, blue, white_point)
    , to_xyz(kMathNoInit)
    , from_xyz(kMathNoInit)
{
    to_xyz   = chromaticities.matrix_to_xyz();
    from_xyz = inverse(to_xyz);
}

// convert matrix
inline double3x3 ColorSpace::convert_matrix(
    const ColorSpace&          from,
    const ColorSpace&          to,
    EChromaticAdaptationMethod method
)
{
    if (method == EChromaticAdaptationMethod::XYZScaling)
    {
        return from.to_xyz * to.from_xyz;
    }
    else
    {
        const auto from_white_xyz = double3(1.0) * from.to_xyz;
        const auto to_white_xyz   = double3(1.0) * to.to_xyz;

        if (all(nearly_equal(from_white_xyz, to_white_xyz, 1.e-7)))
        {
            return from.to_xyz * to.from_xyz;
        }
        else
        {
            auto adaptation_matrix = Chromaticities::chromatic_adaptation_matrix(
                method,
                from_white_xyz,
                to_white_xyz
            );

            return from.to_xyz * adaptation_matrix * to.from_xyz;
        }
    }
}

// convert color
inline double3 ColorSpace::color_to_XYZ(const double3& color) const
{
    return mul(color, to_xyz);
}
inline double3 ColorSpace::color_from_XYZ(const double3& XYZ) const
{
    return mul(XYZ, from_xyz);
}
} // namespace math
} // namespace skr