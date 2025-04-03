using SB;
using SB.Core;
using Serilog;

[TargetScript]
public static class SkrResourceCompiler
{
    static SkrResourceCompiler()
    {
        var ShaderCompiler = Engine.Program("SkrResourceCompiler")
            .EnableUnityBuild()
            .Depend(Visibility.Public, "SkrToolCore")
            .Depend(Visibility.Public, "SkrTextureCompiler")
            .Depend(Visibility.Public, "SkrShaderCompiler", "SkrGLTFTool")
            .Depend(Visibility.Public, "SkrAnimTool")
            .AddCppFiles("**.cpp");
    }
}