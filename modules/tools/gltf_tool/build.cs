using SB;
using SB.Core;
using Serilog;

[TargetScript]
public static class SkrGLTFTool
{
    static SkrGLTFTool()
    {
        Engine.Module("SkrGLTFTool", "GLTFTOOL")
            .EnableUnityBuild()
            .Depend(Visibility.Public, "SkrMeshCore")
            .IncludeDirs(Visibility.Public, "include")
            .AddCppFiles("src/**.cpp")
            .AddMetaHeaders("include/**.hpp")
            .Require("cgltf", new PackageConfig { Version = new Version(1, 13, 0) })
            .Depend(Visibility.Public, "cgltf@cgltf");
    }
}