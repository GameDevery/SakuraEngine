#pragma once
#include "./rtm_conv.hpp"
#include "../gen/misc/quat.hpp"

namespace skr
{
inline namespace math
{
template <ECoordinateSystemHand kCoordHand, EAxis3 kCrossAxis, EAxis3 kUpAxis, EAxis3 kForwardAxis>
struct CoordinateSystem {
    static_assert(kCrossAxis != kUpAxis && kCrossAxis != kForwardAxis && kUpAxis != kForwardAxis, "Axes must be distinct.");

    // convert vector3
    static float3  to_skr(const float3& vec);
    static float3  from_skr(const float3& vec);
    static double3 to_skr(const double3& vec);
    static double3 from_skr(const double3& vec);

    // convert rotator
    static RotatorF to_skr(const RotatorF& rot);
    static RotatorF from_skr(const RotatorF& rot);
    static RotatorD to_skr(const RotatorD& rot);
    static RotatorD from_skr(const RotatorD& rot);

    // convert quaternion (by translate to rotator)
    static QuatF to_skr(const QuatF& quat);
    static QuatF from_skr(const QuatF& quat);
    static QuatD to_skr(const QuatD& quat);
    static QuatD from_skr(const QuatD& quat);

    // convert transform
    static TransformF to_skr(const TransformF& transform);
    static TransformF from_skr(const TransformF& transform);
    static TransformD to_skr(const TransformD& transform);
    static TransformD from_skr(const TransformD& transform);

    // convert AFFINE matrix3, if matrix isnot affine, the behavior is undefined
    static float3x3  to_skr(const float3x3& mat);
    static float3x3  from_skr(const float3x3& mat);
    static double3x3 to_skr(const double3x3& mat);
    static double3x3 from_skr(const double3x3& mat);

    // convert AFFINE matrix4, if matrix isnot affine, the behavior is undefined
    static float4x4  to_skr(const float4x4& mat);
    static float4x4  from_skr(const float4x4& mat);
    static double4x4 to_skr(const double4x4& mat);
    static double4x4 from_skr(const double4x4& mat);

private:
    // helper
    static size_t _axis_to_index(EAxis3 axis);
};

//======================utility coordinate system======================
using CoordinateSystem_LeftHand_YUp = CoordinateSystem<
    ECoordinateSystemHand::LeftHand,
    EAxis3::X, /* cross */
    EAxis3::Y, /* up */
    EAxis3::Z  /* forward */
    >;
using CoordinateSystem_LeftHand_ZUp = CoordinateSystem<
    ECoordinateSystemHand::LeftHand,
    EAxis3::Y, /* cross */
    EAxis3::Z, /* up */
    EAxis3::X  /* forward */
    >;
using CoordinateSystem_RightHand_YUp = CoordinateSystem<
    ECoordinateSystemHand::RightHand,
    EAxis3::X, /* cross */
    EAxis3::Y, /* up */
    EAxis3::Z  /* forward */
    >;
using CoordinateSystem_RightHand_ZUp = CoordinateSystem<
    ECoordinateSystemHand::RightHand,
    EAxis3::Y, /* cross */
    EAxis3::Z, /* up */
    EAxis3::X  /* forward */
    >;

//======================some dcc & engine coordinate system======================

// sakura engine coordinate system
using CoordinateSystem_Skr = CoordinateSystem_LeftHand_YUp;

// left hand && y-up coordinate system
using CoordinateSystem_Unity     = CoordinateSystem_LeftHand_YUp;
using CoordinateSystem_LightWave = CoordinateSystem_LeftHand_YUp;
using CoordinateSystem_ZBrush    = CoordinateSystem_LeftHand_YUp;
using CoordinateSystem_Cinema4D  = CoordinateSystem_LeftHand_YUp;

// left hand && z-up coordinate system
using CoordinateSystem_Unreal = CoordinateSystem_LeftHand_ZUp;

// right hand && y-up coordinate system
using CoordinateSystem_Maya             = CoordinateSystem_RightHand_YUp;
using CoordinateSystem_MoDo             = CoordinateSystem_RightHand_YUp;
using CoordinateSystem_SubstancePainter = CoordinateSystem_RightHand_YUp;
using CoordinateSystem_Houdini          = CoordinateSystem_RightHand_YUp;
using CoordinateSystem_Godot            = CoordinateSystem_RightHand_YUp;
using CoordinateSystem_Roblox           = CoordinateSystem_RightHand_YUp;

// right hand && z-up coordinate system
using CoordinateSystem_3DSMax   = CoordinateSystem_RightHand_ZUp;
using CoordinateSystem_Blender  = CoordinateSystem_RightHand_ZUp;
using CoordinateSystem_SketchUp = CoordinateSystem_RightHand_ZUp;
using CoordinateSystem_Source   = CoordinateSystem_RightHand_ZUp;
using CoordinateSystem_AutoCAD  = CoordinateSystem_RightHand_ZUp;

} // namespace math
} // namespace skr