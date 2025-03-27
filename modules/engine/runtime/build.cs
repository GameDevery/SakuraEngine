using System.Diagnostics;
using SB;
using SB.Core;
using Serilog;

[TargetScript]
public static class SkrRT
{
    static SkrRT()
    {
        var SkrRT = Engine
            .Module("SkrRT", "SKR_RUNTIME")
            .Depend(Visibility.Public, "SkrTask")
            .Depend(Visibility.Public, "SkrGraphics")
            .IncludeDirs(Visibility.Public, "include")
            .AddCppFiles("src/**/build.*.cpp")
            .AddCodegenScript("meta/ecs.ts");

        if (BuildSystem.TargetOS == OSPlatform.OSX)
        {
            SkrRT
                // .AddFrameworks(Visibility.Public, "CoreFoundation", "IOKit")
                // .MppFlags(Visibility.Public, "-fno-objc-arc")
                .AddCFiles("src/**/build.*.m")
                .AddCppFiles("src/**/build.*.mm");
        }
    }
}
