using SB;
using SB.Core;
using Serilog;

[TargetScript]
public static class SkrLive2DViewer
{
    static SkrLive2DViewer()
    {
        Engine.AddSetup<Live2DViewerSetup>();

        Engine.Program("Live2DViewer", "LIVE2D_VIEWER")
            .Depend(Visibility.Public, "SkrLive2D", "SkrImGui")
            .Depend(Visibility.Private, "AppSampleCommon")
            .IncludeDirs(Visibility.Private, "./../../common", "include")
            .AddCppFiles("src/main.cpp", "src/live2d_viewer.cpp");
    }
}

public class Live2DViewerSetup : ISetup
{
    public void Setup()
    {
        Install.SDK("Live2DModels", new Dictionary<string, string> {
            { "./", "./../resources/Live2DViewer" }
        }, false).Wait();
    }
}