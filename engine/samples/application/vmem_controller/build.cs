using SB;
using SB.Core;

[TargetScript]
public static class VMemController
{
    static VMemController()
    {
        Engine.Program("VMemController", "VMEM_CONTROLLER")
            .EnableUnityBuild()
            .Depend(Visibility.Public, "SkrRenderer", "SkrImGui")
            .Depend(Visibility.Private, "AppSampleCommon")
            .IncludeDirs(Visibility.Private, "./../../common")
            .AddCppFiles("*.cpp");
    }
}