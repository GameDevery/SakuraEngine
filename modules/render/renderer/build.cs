using SB;
using SB.Core;
using Serilog;

[TargetScript]
public static class SkrRenderer
{
    static SkrRenderer()
    {
        Engine.Module("SkrRenderer")
            .Depend(Visibility.Public, "SkrSystem", "SkrSceneCore", "SkrRenderGraph")
            .IncludeDirs(Visibility.Public, "include")
            .AddCppFiles("src/*.cpp", "src/allocators/*.cpp")
            .AddCppFiles(new CFamilyFileOptions { UnityGroup = "resources" }, "src/resources/*.cpp")
            .AddCppSLFiles("shaders/**.cppsl")
            .CppSLOutputDirectory("resources/shaders")
            .CppSL_IncludeDirs(Visibility.Public, "shader_common")
            .AddMetaHeaders("include/**.h", "include/**.hpp"); // codegen
    }
}