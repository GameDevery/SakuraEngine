using SB;
using SB.Core;

[TargetScript]
public static class CGPUSamples
{
    static CGPUSamples()
    {
        Engine.Program("CGPUMandelbrot")
            .EnableUnityBuild()
            .Depend(Visibility.Public, "SkrRT")
            .IncludeDirs(Visibility.Private, Path.Combine(SourceLocation.Directory(), "./../../common"))
            .AddCFiles("mandelbrot/*.c");

        Engine.Program("CGPUIndexedInstance")
            .EnableUnityBuild()
            .Depend(Visibility.Public, "SkrRT")
            .IncludeDirs(Visibility.Private, Path.Combine(SourceLocation.Directory(), "./../../common"))
            .AddCFiles("indexed-instance/*.c");

        Engine.Program("CGPUTexture")
            .EnableUnityBuild()
            .Depend(Visibility.Public, "SkrRT")
            .IncludeDirs(Visibility.Private, Path.Combine(SourceLocation.Directory(), "./../../common"))
            .AddCFiles("texture/texture.c");

        Engine.Program("CGPUTiledTexture")
            .EnableUnityBuild()
            .Depend(Visibility.Public, "SkrRT")
            .IncludeDirs(Visibility.Private, Path.Combine(SourceLocation.Directory(), "./../../common"))
            .AddCFiles("texture/tiled_texture.c");
    }
}