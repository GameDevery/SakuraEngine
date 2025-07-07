#include "SkrTestFramework/framework.hpp"
#include "SkrBase/math.h"
#include "SkrAnim/ozz/base/maths/soa_float.h"
#include "SkrAnim/ozz/base/maths/soa_quaternion.h"

TEST_CASE("Quaternion")
{
    using namespace ozz::math;
    SUBCASE("SIMDQuat")
    {
        const SoaQuaternion a = SoaQuaternion::Load(
            ozz::math::simd_float4::Load(.70710677f, 0.f, 0.f, .382683432f),
            ozz::math::simd_float4::Load(0.f, 0.f, .70710677f, 0.f),
            ozz::math::simd_float4::Load(0.f, 0.f, 0.f, 0.f),
            ozz::math::simd_float4::Load(.70710677f, 1.f, .70710677f, .9238795f)
        );
        const SoaQuaternion b = SoaQuaternion::Load(
            ozz::math::simd_float4::Load(0.f, .70710677f, 0.f, -.382683432f),
            ozz::math::simd_float4::Load(0.f, 0.f, .70710677f, 0.f),
            ozz::math::simd_float4::Load(0.f, 0.f, 0.f, 0.f),
            ozz::math::simd_float4::Load(1.f, .70710677f, .70710677f, .9238795f)
        );
        const SoaQuaternion denorm =
            SoaQuaternion::Load(ozz::math::simd_float4::Load(.5f, 0.f, 2.f, 3.f), ozz::math::simd_float4::Load(4.f, 0.f, 6.f, 7.f), ozz::math::simd_float4::Load(8.f, 0.f, 10.f, 11.f), ozz::math::simd_float4::Load(12.f, 1.f, 14.f, 15.f));
    }
}