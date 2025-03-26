using SB;
using SB.Core;
using Serilog;

[TargetScript]
public static class SkrRenderer
{
    static SkrRenderer()
    {
        Engine.Module("SkrRenderer")
            .Depend(Visibility.Public, "SkrScene", "SkrRenderGraph", "SkrImGui")
            .IncludeDirs(Visibility.Public, Path.Combine(SourceLocation.Directory(), "include"))
            .AddCppFiles("src/*.cpp")
            .AddCppFiles("src/resources/*.cpp") // {unity_group = "resources"})
            .AddMetaHeaders("include/**.h", "include/**.hpp"); // codegen
    }
}