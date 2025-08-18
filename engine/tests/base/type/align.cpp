#include "SkrBase/misc/traits.hpp"
#include "SkrTestFramework/framework.hpp"

class CommonTypeAlignTests
{
};

TEST_CASE_METHOD(CommonTypeAlignTests, "align")
{
    alignas(16) float a[4] = { 1, 2, 3, 4 };
    auto              ptr  = skr::assume_aligned<16>(a);
    EXPECT_EQ(ptr, a);
}
