using SB;
using SB.Core;
using Serilog;

[TargetScript]
[ImageCoderDoctor]
public static class SkrImageCoder
{
    static SkrImageCoder()
    {
        var ImageCoder = Engine.Module("SkrImageCoder")
            .EnableUnityBuild()
            .Require("zlib", new PackageConfig { Version = new Version(1, 2, 8) })
            .Depend(Visibility.Private, "zlib@zlib")
            .Depend(Visibility.Public, "SkrRT")
            .IncludeDirs(Visibility.Public, "include")
            .IncludeDirs(Visibility.Private, "turbojpeg")
            .IncludeDirs(Visibility.Private, "libpng/1.5.2")
            .AddCppFiles("src/**.cpp");
        
        if (BuildSystem.TargetOS == OSPlatform.OSX)
        {
            ImageCoder.Link(Visibility.Public, "png");
            ImageCoder.Link(Visibility.Public, "turbojpeg");
        }

        if (BuildSystem.TargetOS == OSPlatform.Windows)
        {
            ImageCoder.Link(Visibility.Public, "libpng15_static")
                .Link(Visibility.Public, "turbojpeg_static")
                .MSVC_NoDefaultLibrary(Visibility.Private, "libcmt");
        }
    }
}

public class ImageCoderDoctor : DoctorAttribute
{
    public override bool Check()
    {
        Task.WaitAll(
            Install.SDK("libpng"),
            Install.SDK("turbojpeg")
        );
        return true;
    }
    public override bool Fix() 
    { 
        Log.Fatal("image coder sdks install failed!");
        return true; 
    }
    public static Task? Installation;
}