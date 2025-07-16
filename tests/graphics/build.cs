using SB;
using SB.Core;

[TargetScript]
public static class GraphicsTests
{
    static GraphicsTests()
    {
        // CGPU Memory Pool Test
        var memPoolTest = Engine.Program("CGPUMemoryPoolTest")
            .TargetType(TargetType.Executable)
            .EnableUnityBuild()
            .Depend(Visibility.Public, "SkrRT")           // 包含 SkrGraphics/CGPU
            .AddCppFiles("memory_pool_test.cpp");
    }
}