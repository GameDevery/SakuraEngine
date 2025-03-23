using SB;
using SB.Core;
using Serilog;

[TargetScript]
[SkrCoreDoctor]
public static class SkrCore
{
    static SkrCore()
    {
        var DependencyGraph = Engine.StaticComponent("SkrDependencyGraph", "SkrCore")
            .OptimizationLevel(OptimizationLevel.Fastest)
            .Depend(Visibility.Public, "SkrBase")
            .Require("lemon", new PackageConfig { Version = new Version(1, 3, 1) })
            .Depend(Visibility.Private, "lemon@lemon")
            .AddFiles("src/graph/build.*.cpp");
        
        var SkrString = Engine.StaticComponent("SkrString", "SkrCore")
            .OptimizationLevel(OptimizationLevel.Fastest)
            .Depend(Visibility.Public, "SkrBase")
            .Defines(Visibility.Public, "OPEN_STRING_API=")
            .AddFiles("src/string/build.*.cpp");

        var SkrSimpleAsync = Engine.StaticComponent("SkrSimpleAsync", "SkrCore")
            .OptimizationLevel(OptimizationLevel.Fastest)
            .Depend(Visibility.Public, "SkrBase")
            .AddFiles("src/async/build.*.cpp");

        var SkrArchive = Engine.StaticComponent("SkrArchive", "SkrCore")
            .OptimizationLevel(OptimizationLevel.Fastest)
            .Depend(Visibility.Public, "SkrBase")
            .Require("yyjson", new PackageConfig { Version = new Version(0, 9, 0) })
            .Depend(Visibility.Private, "yyjson@yyjson")
            .AddFiles("src/archive/build.*.cpp");

        var SkrCore = Engine.Module("SkrCore")
            .Require("phmap", new PackageConfig { Version = new Version(1, 3, 11) })
            .Depend(Visibility.Public, "phmap@phmap")
            .Depend(Visibility.Private, "mimalloc")
            .Depend(Visibility.Public, "SkrProfile")
            // TODO: STATIC COMPONENTS, JUST WORKAROUND NOW, WE NEED TO DEPEND THEM AUTOMATICALLY LATTER
            .Depend(Visibility.Public, "SkrDependencyGraph", "SkrString", "SkrSimpleAsync", "SkrArchive")
            // END WORKAROUNDS
            .IncludeDirs(Visibility.Public, Path.Combine(SourceLocation.Directory(), "include"))
            .Defines(Visibility.Private, "SKR_MEMORY_IMPL")
            // Core Files
            .AddFiles("src/core/build.*.c", "src/core/build.*.cpp")
            // RTTR Files
            .AddFiles("src/rttr/build.*.cpp")
            // Codegen Files
            .AddFiles(
                "include/SkrRTTR/iobject.hpp",
                "include/SkrRTTR/scriptble_object.hpp",
                "include/SkrRTTR/export/export_data.hpp"
            )
            .AddCodegenScript("meta/basic.ts")
            .AddCodegenScript("meta/rttr.ts")
            .AddCodegenScript("meta/serialize.ts")
            .AddCodegenScript("meta/proxy.ts")
            // TODO: REMOVE THIS
            .Depend(Visibility.Public, "SDL2");

        if (BuildSystem.TargetOS == OSPlatform.Windows)
            SkrCore.Link(Visibility.Private, "user32", "shell32", "Ole32", "Shlwapi");
        else
            SkrCore.Link(Visibility.Private, "pthread");

        SkrCore.BeforeBuild((Target) => SkrCoreDoctor.Installation!.Wait());
    }
}

public class SkrCoreDoctor : DoctorAttribute
{
    public override bool Check()
    {
        Installation = Install.SDK("SDL2");
        return true;
    }
    public override bool Fix() 
    { 
        Log.Fatal("core sdks install failed!");
        return true; 
    }
    public static Task? Installation;
}