#include "SkrAnim/ozz/base/maths/internal/simd_math_config.h"
#include "SkrTestFramework/framework.hpp"
#include "SkrBase/math.h"
#include "SkrAnim/ozz/base/maths/soa_float.h"
#include "SkrAnim/ozz/base/maths/soa_float4x4.h"
#include "helper.hpp"

// ozz SOA uses a packed type to batchly process 4 items with one SIMD instruction.

TEST_CASE("SoaFloat")
{
    using namespace skr;
    SUBCASE("SIMDFloat")
    {
        // equivalent to float
        // but with 4 items packed in a batch
        CHECK(sizeof(ozz::math::SimdFloat4) == 16); // __m128 is 16 bytes, 16 * 8 = 128 bits
        ozz::math::SimdFloat4 a = ozz::math::simd_float4::Load(1.0f, 2.0f, 3.0f, 4.0f);
        ozz::math::SimdFloat4 b = ozz::math::simd_float4::Load(5.0f, 6.0f, 7.0f, 8.0f);
        ozz::math::SimdFloat4 c = a + b; // batchly calculate 4 items
        union
        {
            ozz::math::SimdFloat4 simd;
            float values[4];
        } u = { c };
        CHECK(u.values[0] == 6.0f);
        CHECK(u.values[1] == 8.0f);
        CHECK(u.values[2] == 10.0f);
        CHECK(u.values[3] == 12.0f);
        // alternatively, use helper function
        CHECK(ozz::check_eq(c, 6.0f, 8.0f, 10.0f, 12.0f));
    }
    SUBCASE("SoaFloat3")
    {

        CHECK(sizeof(ozz::math::SoaFloat3) == sizeof(ozz::math::SimdFloat4) * 3);
    }
}

TEST_CASE("SoaFloat4")
{
    // SoaFloat4 is a structure that contains 4 SimdFloat4, each representing a vector of 4 floats.
    // use it as a simple float4 vector, but with 4 items packed in a batch

    using namespace ozz::math;
    SUBCASE("SoaFloat4")
    {
        const SoaFloat4 zero = SoaFloat4::zero();
        CHECK(ozz::check_eq(zero, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f));
        const SoaFloat4 one = SoaFloat4::one();
        CHECK(ozz::check_eq(one, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f));
        // assume we have 4 float4 vectors
        // [1, 5, 9, 13]
        // [2, 6, 10, 14]
        // [3, 7, 11, 15]
        // [4, 8, 12, 16]
        SoaFloat4 v1 = SoaFloat4::Load(
            simd_float4::Load(1.0f, 2.0f, 3.0f, 4.0f),
            simd_float4::Load(5.0f, 6.0f, 7.0f, 8.0f),
            simd_float4::Load(9.0f, 10.0f, 11.0f, 12.0f),
            simd_float4::Load(13.0f, 14.0f, 15.0f, 16.0f));

        // and 4 more vectors
        // [17, 21, 25, 29]
        // [18, 22, 26, 30]
        // [19, 23, 27, 31]
        // [20, 24, 28, 32]
        SoaFloat4 v2 = SoaFloat4::Load(
            simd_float4::Load(17.0f, 18.0f, 19.0f, 20.0f),
            simd_float4::Load(21.0f, 22.0f, 23.0f, 24.0f),
            simd_float4::Load(25.0f, 26.0f, 27.0f, 28.0f),
            simd_float4::Load(29.0f, 30.0f, 31.0f, 32.0f));
        // we can add them together
        SoaFloat4 result = v1 + v2;
        // the result should be
        // [1, 5, 9, 13] + [17, 21, 25, 29] = [18, 26, 34, 42]
        // [2, 6, 10, 14] + [18, 22, 26, 30] = [20, 28, 36, 44]
        // [3, 7, 11, 15] + [19, 23, 27, 31] = [22, 30, 38, 46]
        // [4, 8, 12, 16] + [20, 24, 28, 32] = [24, 32, 40, 48]
        CHECK(ozz::check_eq(result, 18.0f, 20.0f, 22.0f, 24.0f, 26.0f, 28.0f, 30.0f, 32.0f, 34.0f, 36.0f, 38.0f, 40.0f, 42.0f, 44.0f, 46.0f, 48.0f));
    }
}

TEST_CASE("SoaFloat4x4")
{
    // SoaFloat4x4 is a structure that contains 4 SoaFloat4, each representing a column of a 4x4 matrix.
    // use it as a simple 4x4 matrix, but with 4 items packed in a batch
    using namespace ozz::math;
    SUBCASE("SoaFloat4x4")
    {
        const SoaFloat4x4 identity = SoaFloat4x4::identity();
    }
}