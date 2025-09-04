using SB;
using SB.Core;

[TargetScript]
public static class AppSampleCommon
{
    static AppSampleCommon()
    {
        Engine.Target("lodepng")
            .TargetType(TargetType.Static)
            .IncludeDirs(Visibility.Public, "lodepng")
            .AddCFiles("lodepng/lodepng.c");

        Engine.Target("AppSampleCommon")
            .TargetType(TargetType.HeaderOnly)
            .Require("SDL3", new PackageConfig { Version = new Version(1, 0, 0) })
            .Depend(Visibility.Public, "SDL3@SDL3")
            .IncludeDirs(Visibility.Public, "include")
            .Depend(Visibility.Public, "SkrCore", "SkrGraphics");
    }
}