#include "container_test_types.hpp"
#include "SkrTestFramework/framework.hpp"

TEST_CASE("test format")
{
    using namespace skr::test_container;

    // escaped braces
    REQUIRE_EQ(StringView{ u8"{}" }, format(u8"{{}}"));

    // integer
    REQUIRE_EQ(StringView{ u8"0" }, format(u8"{}", 0));
    REQUIRE_EQ(StringView{ u8"00255" }, format(u8"{:05d}", 255));
    REQUIRE_EQ(StringView{ u8"ff" }, format(u8"{:x}", 255));
    REQUIRE_EQ(StringView{ u8"-0xff" }, format(u8"{:#x}", -255));
    REQUIRE_EQ(StringView{ u8"_1762757171" }, format(u8"_{}", 1762757171ull));

    // float
    REQUIRE_EQ(StringView{ u8"3.14" }, format(u8"{}", 3.14f));
    REQUIRE_EQ(StringView{ u8"3.1" }, format(u8"{:.1f}", 3.14f));
    REQUIRE_EQ(StringView{ u8"-3.14000" }, format(u8"{:.5f}", -3.14f));
    REQUIRE_EQ(StringView{ u8"-100" }, format(u8"{}", -99.999999999));
    REQUIRE_EQ(StringView{ u8"60.004" }, format(u8"{}", 60.004));
    REQUIRE_EQ(StringView{ u8"inf" }, format(u8"{}", std::numeric_limits<float>::infinity()));
    REQUIRE_EQ(StringView{ u8"-inf" }, format(u8"{}", -std::numeric_limits<double>::infinity()));
    REQUIRE_EQ(StringView{ u8"nan" }, format(u8"{}", std::numeric_limits<float>::quiet_NaN()));

    // pointer
    REQUIRE_EQ(StringView{ u8"nullptr" }, format(u8"{}", nullptr));
    REQUIRE_EQ(StringView{ u8"0x00000000075bcd15" }, format(u8"{}", reinterpret_cast<void*>(123456789)));

    // string
    REQUIRE_EQ(StringView{ u8"繁星明 😀😀" },
               format(u8"{}{}明{}{}", u8"繁",
                      StringView{ u8"星" },
                      String{ u8" 😀" },
                      String{ u8"😀" }.c_str()));
}