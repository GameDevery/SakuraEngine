using SB;
using SB.Core;
using Serilog;

[TargetScript]
public static class AnimSampleCore
{
    static AnimSampleCore()
    {
        Engine.Module("AnimSampleCore", "ANIM_SAMPLE_CORE")
            .Depend(Visibility.Public, "SceneRenderer")
            .Depend(Visibility.Public, "SkrTextureCompiler")
            .Depend(Visibility.Public, "SkrAnimTool")
            .Depend(Visibility.Public, "SkrMeshTool")
            .IncludeDirs(Visibility.Public, "include")
            .AddCppFiles("src/*.cpp");
    }
}