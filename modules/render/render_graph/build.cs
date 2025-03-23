using SB;
using SB.Core;
using Serilog;

[TargetScript]
public static class SkrRenderGraph
{
    static SkrRenderGraph()
    {
        Engine
            .Module("SkrRenderGraph")
            .EnableUnityBuild()
            .Depend(Visibility.Public, "SkrRT")
            .IncludeDirs(Visibility.Public, Path.Combine(SourceLocation.Directory(), "include"))
            .AddFiles("src/frontend/*.cpp", "src/graphviz/*.cpp")
            .AddFiles("src/backend/*.cpp") // {unity_group = "backend"}
            .AddFiles("src/phases/*.cpp"); // {unity_group = "backend"}
    }
}