using SB;
using SB.Core;
using Serilog;

[TargetScript]
public static class SkrRenderer
{
    static SkrRenderer()
    {
        Engine
            .Module("SkrRenderer")
            .Depend(Visibility.Public, "SkrScene", "SkrRenderGraph", "SkrImGui")
            .IncludeDirs(Visibility.Public, Path.Combine(SourceLocation.Directory(), "include"))
            .AddFiles("src/*.cpp")
            .AddFiles("src/resources/*.cpp") // {unity_group = "resources"})
            .AddFiles("include/**.h", "include/**.hpp"); // codegen
    }
}