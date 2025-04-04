using SB;
using SB.Core;
using Serilog;

[TargetScript]
public static class SkrAnimTool
{
    static SkrAnimTool()
    {
        Engine.Module("SkrAnimTool", "SKR_ANIMTOOL")
            // .EnableUnityBuild()
            // .Exception(true)
            .Depend(Visibility.Public, "SkrMeshCore", "SkrAnim")
            .IncludeDirs(Visibility.Public, "include", "ozz")
            .IncludeDirs(Visibility.Private, "src")
            .AddCppFiles("src/*.cc") //, {unity_group = "utils"})
            .AddCppFiles("src/tools/*.cc", "src/*.cpp") //, {unity_group = "tool"})
            .AddCppFiles("src/gltf/**.cc", "src/gltf/**.cpp") //, {unity_ignored = false})
            .UsePrivatePCH("src/pch.hpp")

            .AddMetaHeaders("include/**.h")

            .Require("cgltf", new PackageConfig { Version = new Version(1, 13, 0) })
            .Depend(Visibility.Public, "cgltf@cgltf")

            .Require("tinygltf", new PackageConfig { Version = new Version(2, 8, 14) })
            .Depend(Visibility.Public, "tinygltf@tinygltf");
    }
}