using SB;
using SB.Core;
using Serilog;

[TargetScript]
public static class AnimDebugRuntime
{
    static AnimDebugRuntime()
    {
        // font is already installed by ImGui module
        Engine.Module("AnimDebugRuntime", "ANIM_DEBUG_RUNTIME")
            .Depend(Visibility.Public, "SkrImGui", "SkrAnim")
            .IncludeDirs(Visibility.Public, "include")
            .Depend(Visibility.Public, "AppSampleCommon")
            .AddCppFiles("src/**.cpp")
            .AddMetaHeaders(
                "include/AnimDebugRuntime/animation.h",
                "include/AnimDebugRuntime/renderer.h"
            );
    }
}