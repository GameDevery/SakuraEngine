using SB;
using SB.Core;
using Serilog;

[TargetScript]
public static class SkrMeshCore
{
    static SkrMeshCore()
    {
        Engine.Module("SkrMeshCore", "MESH_CORE")
            .EnableUnityBuild()
            .Depend(Visibility.Public, "SkrToolCore", "SkrRenderer")
            .IncludeDirs(Visibility.Public, "include")
            .AddCppFiles("src/**.cpp")
            .AddMetaHeaders("include/**.hpp")
            .Require("MeshOptimizer", new PackageConfig { Version = new Version(0, 25, 0) })
            .Depend(Visibility.Public, "MeshOptimizer@MeshOptimizer");
    }
}