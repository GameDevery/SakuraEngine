using SB;
using SB.Core;
using System.Runtime.CompilerServices;

[TargetScript]
public static class BaseTests
{
    static BaseTests()
    {
        Engine.UnitTest("OSTest")
            .AddCppFiles("os/main.cpp");

        Engine.UnitTest("AlgoTest")
            .AddCppFiles("algo/*.cpp");

        Engine.UnitTest("ContainersTest")
            .AddCppFiles("containers/*.cpp");
    }
}