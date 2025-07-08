using SB;
using SB.Core;
using Serilog;

[TargetScript]
public static class AnimTemp {
    static AnimTemp()
    {
        Engine.Program("AnimTemp", "ANIM_TEMP")
            .Depend(Visibility.Private, "AnimDebugRuntime")
            .AddCppFiles("anim_temp.cpp");
    }
}
