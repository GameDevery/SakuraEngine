#include "SkrBase/types.h"
#include "SkrTestFramework/framework.hpp"

class TypeHashTests
{
};

TEST_CASE_METHOD(TypeHashTests, "MD5")
{
    EXPECT_EQ(skr::StringView{ u8"02" }, skr::format(u8"{:02x}", 2));
    uint8_t hex = 2;
    EXPECT_EQ(skr::StringView{ u8"02" }, skr::format(u8"{:02x}", hex));

    // "MD5 Hash Generator" ->
    // 992927e5b1c8a237875c70a302a34e22
    skr_md5_t MD5    = {};
    auto      string = u8"MD5 Hash Generator";
    skr_make_md5(string, (uint32_t)strlen((const char*)string), &MD5);
    skr_md5_t  MD5_2         = {};
    auto       formatted     = skr::format(u8"{}", MD5);
    const auto formattedCStr = formatted.c_str();
    skr_parse_md5(formattedCStr, &MD5_2);
    EXPECT_EQ(formatted, u8"992927e5b1c8a237875c70a302a34e22");
    EXPECT_EQ(MD5, MD5_2);
}