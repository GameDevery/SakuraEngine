using SB;
using SB.Core;
using System.Runtime.CompilerServices;

[TargetScript]
public static class CoreTests
{
    static CoreTests()
    {
        Engine.UnitTest("JsonTest")
            .AddCppFiles("json/main.cpp");

        Engine.UnitTest("SerdeTest")
            .AddCppFiles("serde/main.cpp");

        Engine.UnitTest("NatvisTest")
            .Depend(Visibility.Public, "SkrRT")
            .AddCppFiles("natvis/*.cpp");

        Engine.UnitTest("DelegateTest")
            .AddCppFiles("delegate/*.cpp");
    }
}