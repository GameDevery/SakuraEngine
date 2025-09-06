using SB;
using SB.Core;
using System.Runtime.CompilerServices;

[TargetScript]
public static class CoreTests
{
    static CoreTests()
    {
        Test.UnitTest("JsonTest")
            .AddCppFiles("json/main.cpp");

        Test.UnitTest("SerdeTest")
            .EnableUnityBuild()
            .AddCppFiles("serde/main.cpp");

        Test.UnitTest("NatvisTest")
            .Depend(Visibility.Public, "SkrCore")
            .AddCppFiles("natvis/*.cpp");

        Test.UnitTest("DelegateTest")
            .AddCppFiles("delegate/*.cpp");
    }
}