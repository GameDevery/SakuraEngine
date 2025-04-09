using SB;
using SB.Core;
using System.Runtime.CompilerServices;

[TargetScript]
public static class MemoryTests
{
    static MemoryTests()
    {
        Engine.UnitTest("SSMTest")
            .AddCppFiles("SSM/*.cpp");
    }
}