using SB;
using SB.Core;
using System.Runtime.CompilerServices;

[TargetScript]
public static class AnimTests
{
    static AnimTests()
    {
        Engine.UnitTest("OzzTest")
            .Depend(Visibility.Public, "SkrAnim")
            .AddCppFiles("ozz/*.cpp"); // 因为ozz-animation是源码引入且有一定的修改，所以将测试代码也一并移植，保证核心功能的正确性
    }
}