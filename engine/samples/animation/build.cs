using SB;
using SB.Core;
using Serilog;

[TargetScript]
public static class AnimSamples
{
    static AnimSamples()
    {
        Engine.Program("AnimReadSkeleton")
            .Depend(Visibility.Private, "AnimDebugRuntime")
            .AddCppFiles("read_skeleton.cpp");

        Engine.Program("AnimReadAnimation")
            .Depend(Visibility.Private, "AnimDebugRuntime")
            .AddCppFiles("read_animation.cpp");

        Engine.Program("AnimDebug")
            .Depend(Visibility.Private, "AnimDebugRuntime")
            .AddCppFiles("debug/src/*.cpp")
            .AddHLSLFiles("debug/shaders/*.hlsl")
            .DXCOutputDirectory("resources/shaders/AnimDebug");

        Engine.Program("AnimSampleCook")
            .Depend(Visibility.Private, "SkrAnim")
            .Depend(Visibility.Private, "SkrMeshTool")
            .Depend(Visibility.Private, "SkrAnimTool")
            .AddCppFiles("cook/*.cpp");
    }
}
