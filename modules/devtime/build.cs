using SB;
using SB.Core;

[TargetScript]
public static class SkrDevCore
{
    static SkrDevCore()
    {
        var SkrDevCore = Engine
            .Module("SkrDevCore", "SKR_DEVCORE")
            .EnableUnityBuild()
            .Depend(Visibility.Public, "SkrImGui")
            .IncludeDirs(Visibility.Public, "include")
            .AddCppFiles("src/**.cpp");
    }
}
