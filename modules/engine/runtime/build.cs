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
            .IncludeDirs(Visibility.Public, Path.Combine(SourceLocation.Directory(), "include"))
            .AddFiles("src/**/build.*.cpp")
            .AddCodegenScript("meta/ecs.ts");

        if (BuildSystem.TargetOS == OSPlatform.OSX)
        {
            SkrRT
                // .AddFrameworks(Visibility.Public, "CoreFoundation", "IOKit")
                // .MppFlags(Visibility.Public, "-fno-objc-arc")
                .AddFiles("src/**/build.*.m", "src/**/build.*.mm");
        }
    }
}
