// Math Operations on 3D Coordinate Transformations
#include "SkrTestFramework/framework.hpp"
#include "SkrBase/math.h"

TEST_CASE("DefaultCoordinate")
{
    using namespace skr;
    {
        // Test default coordinate system
        // x right
        // z forward
        // y up
        auto f = skr::float3::forward();
        CHECK(f[0] == 0.0f);
        CHECK(f[1] == 0.0f);
        CHECK(f[2] == 1.0f);
        auto r = skr::float3::right();
        CHECK(r[0] == 1.0f);
        CHECK(r[1] == 0.0f);
        CHECK(r[2] == 0.0f);
        auto u = skr::float3::up();
        CHECK(u[0] == 0.0f);
        CHECK(u[1] == 1.0f);
        CHECK(u[2] == 0.0f);
    }
}
