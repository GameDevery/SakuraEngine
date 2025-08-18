using SB;
using SB.Core;
using System.Runtime.CompilerServices;

[TargetScript]
public static class BaseTests
{
    static BaseTests()
    {
        Test.UnitTest("OSTest")
            .AddCppFiles("os/main.cpp");

        Test.UnitTest("AlgoTest")
            .AddCppFiles("algo/*.cpp");

        Test.UnitTest("ContainersTest")
            .AddCppFiles("containers/*.cpp");

        Test.UnitTest("MathTest")
            .AddCppFiles("math/*.cpp");

        Test.UnitTest("TypeTest")
            .AddCppFiles("type/*.cpp");

        Test.UnitTest("FileSystemTest")
            .AddCppFiles("filesystem/*.cpp");
    }
}