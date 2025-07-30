using SB;
using SB.Core;

[TargetScript]
public static class ThirdPartyTests
{
    static ThirdPartyTests()
    {
        Engine.UnitTest("ThirdPartyTest_ozz")
            .Depend(Visibility.Public, "SkrAnim")
            .AddCppFiles("ozz/*.cpp"); // 因为ozz-animation是源码引入且有一定的修改，所以将测试代码也计划一并移植，保证核心功能的正确性
        
        Engine.UnitTest("ThirdPartyTest_rtm")
            .Depend(Visibility.Public, "SkrRT")
            .AddCppFiles("rtm/*.cpp"); // rtm是skr::math的核心，需要基本测试防止上游API变更导致的错误

    }
}