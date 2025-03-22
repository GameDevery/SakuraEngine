using SB;
using SB.Core;
using Serilog;

[TargetScript]
[SkrLive2DDoctor]
public static class SkrLive2D
{
    static SkrLive2D()
    {
        var CubismFramework = Engine
            .StaticComponent("CubismFramework", "SkrLive2D")
            .OptimizationLevel(OptimizationLevel.Fastest)
            .Depend(Visibility.Public, "SkrBase")
            .IncludeDirs(Visibility.Private, Path.Combine(SourceLocation.Directory(), "CubismNativeCore/include"))
            .IncludeDirs(Visibility.Private, Path.Combine(SourceLocation.Directory(), "CubismFramework"))
            .IncludeDirs(Visibility.Private, Path.Combine(SourceLocation.Directory(), "CubismFramework/Framework"))
            .AddFiles("CubismFramework/Renderer/**.cpp", "CubismFramework/Framework/**.cpp");

        CubismFramework.BeforeBuild(Target => SkrLive2DDoctor.SDKInstallation!.Wait());
        if (BuildSystem.TargetOS == OSPlatform.Windows)
        {
            CubismFramework.Link(Visibility.Public, "Live2DCubismCore_MD");
        }
        else if (BuildSystem.TargetOS == OSPlatform.OSX)
        {
            CubismFramework.Link(Visibility.Public, "Live2DCubismCore");
        }

        Engine
            .Module("SkrLive2D", "SKR_LIVE2D")
            .Depend(Visibility.Public, "SkrImageCoder", "SkrRenderer")
            .Depend(Visibility.Private, "CubismFramework")
            .IncludeDirs(Visibility.Public, Path.Combine(SourceLocation.Directory(), "include"))
            .IncludeDirs(Visibility.Private, Path.Combine(SourceLocation.Directory(), "CubismNativeCore/include"))
            .IncludeDirs(Visibility.Private, Path.Combine(SourceLocation.Directory(), "CubismFramework"))
            .IncludeDirs(Visibility.Private, Path.Combine(SourceLocation.Directory(), "CubismFramework/Framework"))
            .AddFiles("src/*.cpp");
    }
}

public class SkrLive2DDoctor : DoctorAttribute
{
    public override bool Check()
    {
        SDKInstallation = Install.SDK("CubismNativeCore");
        return true;
    }

    public override bool Fix()
    {
        Log.Fatal("Cubism SDK install failed!");
        return true;
    }
    public static Task? SDKInstallation;
}