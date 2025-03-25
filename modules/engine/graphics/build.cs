using System.Diagnostics;
using SB;
using SB.Core;
using Serilog;

[TargetScript]
[SkrGraphicsDoctor]
public static class SkrGraphics
{
    static SkrGraphics()
    {
        var SkrGraphics = Engine
            .Module("SkrGraphics")
            .Depend(Visibility.Public, "SkrCore")
            .Depend(Visibility.Private, "VulkanHeaders")
            .IncludeDirs(Visibility.Public, Path.Combine(SourceLocation.Directory(), "include"))
            .AddCFiles("src/build.*.c")
            .AddCppFiles("src/build.*.cpp");

        if (BuildSystem.TargetOS == OSPlatform.OSX)
        {
            SkrGraphics
                .AddCFiles("src/build.*.m") // mxflags: "-fno-objc-arc"
                .AddCppFiles("src/build.*.mm") // mxflags: "-fno-objc-arc"
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

        SkrGraphics.BeforeBuild((Target) => SkrGraphicsDoctor.Installation!.Wait() );
    }
}

public class SkrGraphicsDoctor : DoctorAttribute
{
    public override bool Check()
    {
        if (BuildSystem.TargetOS == OSPlatform.Windows)
        {
            Stopwatch sw = new();
            sw.Start();
            Installation = Task.WhenAll(
                Install.SDK("dxc-2025_02_21"),
                Install.SDK("amdags"),
                Install.SDK("nvapi"),
                Install.SDK("nsight"),
                Install.SDK("WinPixEventRuntime")
            );
            sw.Stop();
            Log.Information("graphics sdks install took {0} ms", sw.ElapsedMilliseconds);
        }
        return true;
    }
    public override bool Fix() 
    { 
        Log.Fatal("graphics sdks install failed!");
        return true; 
    }
    public static Task? Installation;
}