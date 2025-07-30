#include "SkrTestFramework/framework.hpp"
#include "rtm/types.h"

#include <rtm/scalarf.h>
#include <rtm/scalard.h>
#include <rtm/vector4f.h>
#include <rtm/vector4d.h>

#if defined(RTM_SSE2_INTRINSICS)
    #include <xmmintrin.h>
#endif

TEST_CASE("basic-scalar")
{
    using namespace rtm;

    SUBCASE("scalar")
    {
#if defined(RTM_SSE2_INTRINSICS)
        scalarf a = scalar_set(1.0f);
        // the implementation of scalarf is a wrapper around __m128 when SSE2 is enabled
        CHECK(std::is_same<decltype(a.value), __m128>::value);
        CHECK(sizeof(a) == 16); // __m128 is 16 bytes, 16 * 8 = 128 bits

        scalard b = scalar_set(2.0);
        // the implementation of scalard is a wrapper around __m128d when SSE2 is enabled
        CHECK(std::is_same<decltype(b.value), __m128d>::value);
        CHECK(sizeof(b) == 16); // __m128d is 16 bytes, 16 * 8 = 128 bits
#endif
    }
}

TEST_CASE("basic-vector")
{
    using namespace rtm;

    SUBCASE("vector4")
    {
#if defined(RTM_SSE2_INTRINSICS)
        std::array<float, 4> values = { 1.0f, 2.0f, 3.0f, 4.0f };
        vector4f v = rtm::vector_load(values.data());
        // the implementation of vector4f is a wrapper around __m128 when SSE2 is enabled
        CHECK(std::is_same<decltype(v), __m128>::value);
        CHECK(sizeof(v) == 16); // __m128 is 16 bytes, 16 * 8 = 128 bits

        std::array<double, 4> dvalues = { 1.0, 2.0, 3.0, 4.0 };
        vector4d dv = rtm::vector_load(dvalues.data());
        // vector4d is the combinamtion of 2 __m128d when SSE2 is enabled
        CHECK(sizeof(dv) == 32); // __m128d is 16 bytes, 2 * 16 * 8 = 256 bits
#endif
    }
}