using SB;
using SB.Core;

[TargetScript]
public static class ModelViewer
{
    static ModelViewer()
    {
        Engine.Program("SkrModelViewer", "SKR_MODEL_VIEWER")
            .EnableUnityBuild()
            .Depend(Visibility.Public, "SkrRenderer", "SkrSystem", "SkrMeshTool")
            .Depend(Visibility.Private, "AppSampleCommon")
            .IncludeDirs(Visibility.Private, "./../../common")
            .AddCppFiles("*.cpp");
    }
}