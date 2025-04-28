using SB;
using SB.Core;
using Serilog;

[TargetScript]
public static class SkrTextureCompiler
{
    static SkrTextureCompiler()
    {
        Engine.Module("SkrTextureCompiler")
            .EnableUnityBuild()
            .Depend(Visibility.Public, "SkrRenderer", "SkrImageCoder", "SkrToolCore")
            .IncludeDirs(Visibility.Public, "include")
            .AddMetaHeaders("include/**.hpp")
            .AddCppFiles("src/**.cpp")
            .AddISPCFiles("src/ispc/**.ispc");
    }
}