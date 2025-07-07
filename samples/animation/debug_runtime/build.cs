using SB;
using SB.Core;
using Serilog;

[TargetScript]
public static class AnimDebug 
{
    static AnimDebug()
    {
        Engine.Program("AnimDebug", "ANIM_DEBUG")
            .Depend(Visibility.Private, "AnimDebugRuntime")
            .IncludeDirs(Visibility.Private, "include")
            .AddCppFiles("src/*.cpp")
            .AddHLSLFiles("shaders/*.hlsl")
            .DXCOutputDirectory("resources/shaders/AnimDebug");
    }
}