using SB;
using SB.Core;
using Serilog;

[TargetScript]
public static class SkrAnim
{
    static SkrAnim()
    {
        Engine.Module("SkrOzz")
            .EnableUnityBuild()
            .OptimizationLevel(OptimizationLevel.Fastest)
            .Depend(Visibility.Public, "SkrRT")
            .IncludeDirs(Visibility.Public, "ozz")
            .IncludeDirs(Visibility.Private, "ozz_src")
            .AddCppFiles("ozz_src/**.cc")
            .UsePrivatePCH("ozz/SkrAnim/ozz/*.h");

        Engine.Module("SkrAnim")
            .EnableUnityBuild()
            .Depend(Visibility.Public, "SkrOzz", "SkrRenderer")
            .IncludeDirs(Visibility.Public, "include")
            .IncludeDirs(Visibility.Public, "ozz")
            .AddCppFiles("src/**.cpp")
            .AddMetaHeaders("include/**.h", "include/**.hpp")
            .UsePrivatePCH("src/pch.hpp");
    }
}