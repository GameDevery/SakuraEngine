using SB;
using SB.Core;
using Serilog;

[TargetScript]
public static class AnimViewModule 
{
    static AnimViewModule()
    {
        // font is already installed by ImGui module
        Engine.Module("AnimView", "ANIM_VIEW")
            .Depend(Visibility.Public, "SkrImGui", "SkrAnim")
            .IncludeDirs(Visibility.Public, "include")
            .Depend(Visibility.Public, "AppSampleCommon")
            .AddCppFiles("src/**.cpp");
            // .AddMetaHeaders(
            //     "include/AnimView/animation.h",
            //     "include/AnimView/renderer.h"
            // );
    }
}