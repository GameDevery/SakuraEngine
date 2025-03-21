using SB;
using SB.Core;

[TargetScript]
public static class SkrGraphics
{
    static SkrGraphics()
    {
        var SkrGraphics = Engine
            .Module("SkrGraphics")
            .Depend(Visibility.Public, "SkrCore")
            .Depend(Visibility.Private, "VulkanHeaders")
            .IncludeDirs(Visibility.Public, Path.Combine(SourceLocation.Directory(), "include"))
            .AddFiles("src/build.*.c", "src/build.*.cpp");

        if (BuildSystem.TargetOS == OSPlatform.OSX)
        {
            SkrGraphics
                .AddFiles("src/build.*.m", "src/build.*.mm") // mxflags: "-fno-objc-arc"
                // .AddFrameworks(Visibility.Public, "CoreFoundation", "Cocoa", "Metal", "IOKit");
                .Defines(Visibility.Private, "VK_USE_PLATFORM_MACOS_MVK");
        }

        if (BuildSystem.TargetOS == OSPlatform.Windows)
        {
            SkrGraphics
                .Defines(Visibility.Private, "UNICODE")
                .Link(Visibility.Private, "nvapi_x64")
                .Link(Visibility.Private, "WinPixEventRuntime");
        }
    }
}