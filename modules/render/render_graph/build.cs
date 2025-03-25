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
            .AddCppFiles("src/frontend/*.cpp", "src/graphviz/*.cpp")
            .AddCppFiles("src/backend/*.cpp") // {unity_group = "backend"}
            .AddCppFiles("src/phases/*.cpp"); // {unity_group = "backend"}
    }
}