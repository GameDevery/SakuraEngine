using SB;
using SB.Core;
using Serilog;

[TargetScript]
public static class SkrScene
{
    static SkrScene()
    {
        Engine.Module("SkrScene", "SKR_SCENE")
            .EnableUnityBuild()
            .Depend(Visibility.Public, "SkrRenderer")
            .Depend(Visibility.Public, "SkrAnim")
            .IncludeDirs(Visibility.Public, "include")
            .AddCppFiles("src/*.cpp")
            .AddMetaHeaders("include/SkrScene/**.h");
    }
}