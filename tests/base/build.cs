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

        Engine.UnitTest("MathTest")
            .AddCppFiles("math/*.cpp");

        Engine.UnitTest("TypeTest")
            .AddCppFiles("type/*.cpp");

        Engine.UnitTest("FileSystemTest")
            .AddCppFiles("filesystem/*.cpp");
    }
}