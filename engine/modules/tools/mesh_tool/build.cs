using SB;
using SB.Core;
using Serilog;

[TargetScript]
public static class SkrMeshTool
{
    static SkrMeshTool()
    {
        Engine.Module("SkrMeshTool", "MESH_TOOL")
            .EnableUnityBuild()
            .Depend(Visibility.Public, "SkrMeshCore")
            .IncludeDirs(Visibility.Public, "include")
            .AddCppFiles("src/**.cpp")
            .AddMetaHeaders("include/**.hpp")
            .Require("cgltf", new PackageConfig { Version = new Version(1, 13, 0) })
            .Depend(Visibility.Public, "cgltf@cgltf")
            .UsePrivatePCH("src/pch.hpp");
    }
}