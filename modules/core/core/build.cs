using SB;
using SB.Core;

[TargetScript]
public static class SkrCore
{
    static SkrCore()
    {
        var DependencyGraph = Project.StaticComponent("SkrDependencyGraph", "SkrCore")
            .OptimizationLevel(OptimizationLevel.O3)
            .Depend(Visibility.Public, "SkrBase")
            .Require("lemon", new PackageConfig { Version = new Version(1, 3, 1) })
            .Depend(Visibility.Private, "lemon@lemon")
            .AddFiles("src/graph/build.*.cpp");
        
        var SkrString = Project.StaticComponent("SkrString", "SkrCore")
            .OptimizationLevel(OptimizationLevel.O3)
            .Depend(Visibility.Public, "SkrBase")
            .Defines(Visibility.Public, "OPEN_STRING_API=")
            .AddFiles("src/string/build.*.cpp");

        var SkrSimpleAsync = Project.StaticComponent("SkrSimpleAsync", "SkrCore")
            .OptimizationLevel(OptimizationLevel.O3)
            .Depend(Visibility.Public, "SkrBase")
            .AddFiles("src/async/build.*.cpp");

        var SkrArchive = Project.StaticComponent("SkrArchive", "SkrCore")
            .OptimizationLevel(OptimizationLevel.O3)
            .Depend(Visibility.Public, "SkrBase")
            .Require("yyjson", new PackageConfig { Version = new Version(0, 9, 0) })
            .Depend(Visibility.Private, "yyjson@yyjson")
            .AddFiles("src/archive/build.*.cpp");

        var SkrCore = Project.Module("SkrCore")
            .Require("phmap", new PackageConfig { Version = new Version(1, 3, 11) })
            .Depend(Visibility.Public, "phmap@phmap")
            .Depend(Visibility.Private, "mimalloc")
            .Depend(Visibility.Public, "SkrProfile")
            // TODO: STATIC COMPONENTS, JUST WORKAROUND NOW, WE NEED TO DEPEND THEM AUTOMATICALLY LATTER
            .Depend(Visibility.Public, "SkrDependencyGraph", "SkrString", "SkrSimpleAsync", "SkrArchive")
            .IncludeDirs(Visibility.Public, "D:/SakuraEngine/D5-NEXT/build/.skr/codegen/SkrCore/codegen")
            // END WORKAROUNDS
            .IncludeDirs(Visibility.Public, Path.Combine(SourceLocation.Directory(), "include"))
            .Defines(Visibility.Private, "SKR_MEMORY_IMPL")
            // Core Files
            .AddFiles("src/core/build.*.c", "src/core/build.*.cpp")
            // RTTR Files
            .AddFiles("src/rttr/build.*.cpp")
            // TODO: REMOVE THIS
            .Depend(Visibility.Public, "SDL2");

        if (BuildSystem.TargetOS == OSPlatform.Windows)
            SkrCore.Link(Visibility.Private, "advapi32", "user32", "shell32", "Ole32", "Shlwapi");
        else
            SkrCore.Link(Visibility.Private, "pthread");
    }
}