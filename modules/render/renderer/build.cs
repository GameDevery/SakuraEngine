using SB;
using SB.Core;
using Serilog;

[TargetScript]
public static class SkrRenderer
{
    static SkrRenderer()
    {
        Engine.Module("SkrRenderer")
            .Depend(Visibility.Public, "SkrSystem", "SkrScene", "SkrRenderGraph")
            .IncludeDirs(Visibility.Public, "include")
            .AddCppFiles("src/*.cpp")
            .AddCppFiles(new CFamilyFileOptions { UnityGroup = "resources" }, "src/resources/*.cpp")
            .AddCppSLFiles("shaders/**.cppsl")
            .CppSLOutputDirectory("resources/shaders")
            .AddMetaHeaders("include/**.h", "include/**.hpp"); // codegen
    }
}