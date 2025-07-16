using SB.Core;
using SB;

[TargetScript]
public static class RenderGraphFakePipelineTest
{
    static RenderGraphFakePipelineTest()
    {
        Engine.Program("RenderGraphFakePipelineTest")
            .TargetType(TargetType.Executable)
            .IncludeDirs(Visibility.Public, ".")
            .AddCppFiles("*.cpp")
            .Depend(Visibility.Private, "SkrRenderGraph")
            .Depend(Visibility.Private, "SkrCore")
            .Depend(Visibility.Private, "SkrBase");
    }
}