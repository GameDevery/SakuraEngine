using SB;
using SB.Core;
using Serilog;

[TargetScript]
public static class SkrScene
{
    static SkrScene()
    {
        Engine.Module("SkrScene")
            .EnableUnityBuild()
            .Depend(Visibility.Public, "SkrRT")
            .IncludeDirs(Visibility.Public, "include")
            .AddCppFiles("src/*.cpp")
            .AddMetaHeaders("include/SkrScene/**.h");
    }
}