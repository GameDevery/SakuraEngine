using SB;
using SB.Core;
using Serilog;

[TargetScript]
public static class GameRuntime
{
    static GameRuntime()
    {
        // font is already installed by ImGui module
        Engine.Module("GameRuntime", "GAME_RUNTIME")
            .EnableUnityBuild()
            .Depend(Visibility.Public, "SkrImGui", "SkrAnim", "SkrRenderer")
            .IncludeDirs(Visibility.Public, "include")
            .Depend(Visibility.Public, "AppSampleCommon")
            .AddCppFiles("src/**.cpp")
            .AddMetaHeaders(
                "include/GameRuntime/gamert.h"
            );
    }
}