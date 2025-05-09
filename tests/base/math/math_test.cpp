#include "SkrTestFramework/framework.hpp"
#include "SkrBase/math.h"

TEST_CASE("Test relative Quat")
{
    using namespace skr;

    // // for quatF
    // {
    //     QuatF quat_child = QuatF::Euler(
    //         radians(10.0f),
    //         radians(20.0f),
    //         radians(30.0f)
    //     );
    //     QuatF quat_parent = QuatF::Euler(
    //         radians(20.0f),
    //         radians(30.0f),
    //         radians(40.0f)
    //     );
    //     QuatF quat_world    = quat_child * quat_parent;
    //     QuatF quat_relative = relative(quat_parent, quat_world);

    //     float3 v{ 1, 1, 1 };
    //     float3 v_child    = v * quat_child;
    //     float3 v_relative = v * quat_relative;
    //     REQUIRE(all(nearly_equal(v_relative, v_child)));
    // }

    // // for QuatD
    // {
    //     QuatD quat_child = QuatD::Euler(
    //         radians(10.0),
    //         radians(20.0),
    //         radians(30.0)
    //     );
    //     QuatD quat_parent = QuatD::Euler(
    //         radians(20.0),
    //         radians(30.0),
    //         radians(40.0)
    //     );
    //     QuatD quat_world    = quat_child * quat_parent;
    //     QuatD quat_relative = relative(quat_parent, quat_world);
    //     REQUIRE(nearly_equal(quat_relative, quat_child));
    // }
}

TEST_CASE("Test relative Transform")
{
    using namespace skr;

    SUBCASE("no neg scale")
    {
        // for TransformF
        {
            QuatF quat_a = QuatF::Euler(
                radians(10.0f),
                radians(20.0f),
                radians(30.0f)
            );
            QuatF quat_b = QuatF::Euler(
                radians(20.0f),
                radians(30.0f),
                radians(40.0f)
            );

            TransformF transform_child{
                quat_a,
                { 1.0f, 2.0f, 3.0f },
                { 3.0f, 2.0f, 1.0f }
            };
            TransformF transform_parent{
                quat_b,
                { 4.0f, 5.0f, 6.0f },
                { 6.0f, 5.0f, 4.0f }
            };

            TransformF transform_world    = transform_child * transform_parent;
            TransformF transform_relative = relative(transform_parent, transform_world);

            REQUIRE(all(nearly_equal(transform_relative.position, transform_child.position)));
            REQUIRE(all(nearly_equal(transform_relative.scale, transform_child.scale)));
        }

        // for TransformD
        {
            QuatD quat_a = QuatD::Euler(
                radians(10.0),
                radians(20.0),
                radians(30.0)
            );
            QuatD quat_b = QuatD::Euler(
                radians(20.0),
                radians(30.0),
                radians(40.0)
            );

            TransformD transform_child{
                quat_a,
                { 1.0, 2.0, 3.0 },
                { 3.0, 2.0, 1.0 }
            };
            TransformD transform_parent{
                quat_b,
                { 4.0, 5.0, 6.0 },
                { 6.0, 5.0, 4.0 }
            };

            TransformD transform_world    = transform_child * transform_parent;
            TransformD transform_relative = relative(transform_parent, transform_world);

            REQUIRE(all(nearly_equal(transform_relative.position, transform_child.position)));
            REQUIRE(all(nearly_equal(transform_relative.scale, transform_child.scale)));
        }
    }

    SUBCASE("with neg scale")
    {
        // for TransformF
        {
            QuatF quat_a = QuatF::Euler(
                radians(10.0f),
                radians(20.0f),
                radians(30.0f)
            );
            QuatF quat_b = QuatF::Euler(
                radians(20.0f),
                radians(30.0f),
                radians(40.0f)
            );

            TransformF transform_child{
                quat_a,
                { 1.0f, 2.0f, 3.0f },
                { -3.0f, 2.0f, 1.0f }
            };
            TransformF transform_parent{
                quat_b,
                { 4.0f, 5.0f, 6.0f },
                { 6.0f, 5.0f, -4.0f }
            };
            TransformF transform_world    = transform_child * transform_parent;
            TransformF transform_relative = relative(transform_parent, transform_world);

            REQUIRE(all(nearly_equal(transform_relative.position, transform_child.position)));
            REQUIRE(all(nearly_equal(transform_relative.scale, transform_child.scale)));
        }

        // for TransformD
        {
            QuatD quat_a = QuatD::Euler(
                radians(10.0),
                radians(20.0),
                radians(30.0)
            );
            QuatD quat_b = QuatD::Euler(
                radians(20.0),
                radians(30.0),
                radians(40.0)
            );

            TransformD transform_child{
                quat_a,
                { 1.0, 2.0, 3.0 },
                { -3.0, 2.0, 1.0 }
            };
            TransformD transform_parent{
                quat_b,
                { 4.0, 5.0, 6.0 },
                { 6.0, 5.0, -4.0 }
            };
            TransformD transform_world    = transform_child * transform_parent;
            TransformD transform_relative = relative(transform_parent, transform_world);

            REQUIRE(all(nearly_equal(transform_relative.position, transform_child.position)));
            REQUIRE(all(nearly_equal(transform_relative.scale, transform_child.scale)));
        }
    }
}