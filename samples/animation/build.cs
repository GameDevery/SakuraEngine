using SB;
using SB.Core;
using Serilog;

[TargetScript]
public static class AnimTemp {
    static AnimTemp()
    {
        Engine.Program("AnimReadSkeleton", "ANIM_TEMP")
            .Depend(Visibility.Private, "AnimDebugRuntime")
            .AddCppFiles("read_skeleton.cpp");

        Engine.Program("AnimReadAnimation", "ANIM_TEMP")
            .Depend(Visibility.Private, "AnimDebugRuntime")
            .AddCppFiles("read_animation.cpp");
    }
}
