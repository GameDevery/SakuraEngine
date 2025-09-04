using SB;
using SB.Core;

[TargetScript]
public static class WebSocketSample
{
    static WebSocketSample()
    {
        if (BuildSystem.TargetOS == OSPlatform.Windows)
        {
            Engine.Program("WebSocketSample")
                .Require("LibHV", new PackageConfig { Version = new Version(1, 3, 3) })
                .Depend(Visibility.Public, "LibHV@LibHV")
                .Depend(Visibility.Public, "SkrCore")
                .IncludeDirs(Visibility.Public, "include")
                .AddMetaHeaders("include/**.hpp")
                .AddCppFiles("src/**.cpp");
        }
    }
}
