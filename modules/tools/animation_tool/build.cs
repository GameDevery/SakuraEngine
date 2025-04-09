using SB;
using SB.Core;
using Serilog;

[TargetScript]
public static class SkrAnimTool
{
    static SkrAnimTool()
    {
        var Gltf2OzzOptions = new CFamilyFileOptions();
        Gltf2OzzOptions.Arguments.CXFlags_ClangCl(Visibility.Private, 
            "-Wno-format-invalid-specifier"
        );

        Engine.Module("SkrAnimTool", "SKR_ANIMTOOL")
            // .EnableUnityBuild()
            // .Exception(true)
            .Depend(Visibility.Public, "SkrMeshCore", "SkrAnim")
            .IncludeDirs(Visibility.Public, "include", "ozz")
            .IncludeDirs(Visibility.Private, "src")
            .AddCppFiles(new CFamilyFileOptions { UnityGroup = "utils" }, "src/*.cc")
            .AddCppFiles(new CFamilyFileOptions { UnityGroup = "tool" }, "src/tools/*.cc", "src/*.cpp")
            .AddCppFiles(Gltf2OzzOptions, "src/gltf/**.cc", "src/gltf/**.cpp")
            .UsePrivatePCH("src/pch.hpp")

            .AddMetaHeaders("include/**.h")

            .Require("cgltf", new PackageConfig { Version = new Version(1, 13, 0) })
            .Depend(Visibility.Public, "cgltf@cgltf")

            .Require("tinygltf", new PackageConfig { Version = new Version(2, 8, 14) })
            .Depend(Visibility.Public, "tinygltf@tinygltf");
    }
}