using SB;
using SB.Core;
using Serilog;

[TargetScript]
public static class SkrLive2D
{
    static SkrLive2D()
    {
        Engine.AddSetup<SkrLive2DSetup>();

        var CubismFramework = Engine
            .StaticComponent("CubismFramework", "SkrLive2D")
            .EnableUnityBuild()
            .OptimizationLevel(OptimizationLevel.Fastest)
            .Depend(Visibility.Public, "SkrBase")
            .IncludeDirs(Visibility.Private, "CubismNativeCore/include", "CubismFramework", "CubismFramework/Framework")
            .AddCppFiles("CubismFramework/Renderer/**.cpp", "CubismFramework/Framework/**.cpp")
            .UsePrivatePCH("CubismFramework/pch.hpp");

        if (BuildSystem.TargetOS == OSPlatform.Windows)
        {
            CubismFramework.Link(Visibility.Public, "Live2DCubismCore_MD");
        }
        else if (BuildSystem.TargetOS == OSPlatform.OSX)
        {
            CubismFramework.Link(Visibility.Public, "Live2DCubismCore");
        }

        var SkrLive2D = Engine
            .Module("SkrLive2D", "SKR_LIVE2D")
            .EnableUnityBuild()
            .Depend(Visibility.Public, "SkrImageCoder", "SkrRenderer")
            .Depend(Visibility.Private, "CubismFramework")
            .IncludeDirs(Visibility.Public, "include")
            .IncludeDirs(Visibility.Private, "CubismNativeCore/include", "CubismFramework", "CubismFramework/Framework")
            .AddCppFiles("src/*.cpp")
            .AddCppSLFiles("shaders/*.cxx");

        SkrLive2D.UsePrivatePCH("src/pch.hpp");
    }
}

public class SkrLive2DSetup : ISetup
{
    public void Setup()
    {
        Install.SDK("CubismNativeCore").Wait();
    }
}