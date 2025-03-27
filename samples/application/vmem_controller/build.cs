using SB;
using SB.Core;

[TargetScript]
public static class VMemController
{
    static VMemController()
    {
        Engine.Program("VMemController", "VMEM_CONTROLLER")
            .EnableUnityBuild()
            .Depend(Visibility.Public, "SkrRenderGraph", "SkrImGui")
            .IncludeDirs(Visibility.Private, Path.Combine(SourceLocation.Directory(), "./../../common"))
            .AddCppFiles("*.cpp");
    }
}