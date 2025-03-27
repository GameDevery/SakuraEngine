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
            .IncludeDirs(Visibility.Private, "./../../common")
            .AddCFiles("mandelbrot/*.c");

        Engine.Program("CGPUIndexedInstance")
            .EnableUnityBuild()
            .Depend(Visibility.Public, "SkrRT")
            .IncludeDirs(Visibility.Private, "./../../common")
            .AddCFiles("indexed-instance/*.c");

        Engine.Program("CGPUTexture")
            .EnableUnityBuild()
            .Depend(Visibility.Public, "SkrRT")
            .IncludeDirs(Visibility.Private, "./../../common")
            .AddCFiles("texture/texture.c");

        Engine.Program("CGPUTiledTexture")
            .EnableUnityBuild()
            .Depend(Visibility.Public, "SkrRT")
            .IncludeDirs(Visibility.Private, "./../../common")
            .AddCFiles("texture/tiled_texture.c");
    }
}