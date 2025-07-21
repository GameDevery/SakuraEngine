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
            .AddCppFiles("src/*.cpp")
            .AddHLSLFiles("shaders/*.hlsl")
            .DXCOutputDirectory("resources/shaders/AnimDebug");
        Engine.Program("AnimDebugNext", "ANIM_DEBUG")
            .Depend(Visibility.Private, "AnimView")
            .AddCppFiles("src_next/*.cpp")
            .AddCppSLFiles("shaders/*.cppsl")
            .DXCOutputDirectory("resources/shaders/AnimDebug");
    }
    
}