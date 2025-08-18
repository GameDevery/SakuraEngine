using SB;
using SB.Core;
using Serilog;

[TargetScript]
public static class Cef
{
    static Cef()
    {
        if (BuildSystem.TargetOS != OSPlatform.Windows)
            return;
            
        Engine.AddSetup<CefSetup>();
        var @this = Engine.Target("cef")
            .TargetType(TargetType.Static)
            .Defines(Visibility.Public, "WRAPPING_CEF_SHARED", "NOMINMAX", "USING_CEF_SHARED=1")
            .Defines(Visibility.Private, "_HAS_EXCEPTIONS=0")
            .IncludeDirs(Visibility.Public, "include")
            .IncludeDirs(Visibility.Private, "./")
            .Link(Visibility.Private, "libcef")
            .AddCppFiles("libcef_dll/**.cc");

        @this.ClangCl_CXFlags(Visibility.Private, "-Wno-undefined-var-template");
    }
}

public class CefSetup : ISetup
{
    public void Setup()
    {
        if (BuildSystem.TargetOS != OSPlatform.Windows)
            return;

        Install.SDK("cef-no-dxc-6778", new Dictionary<string, string> {
            { "Release", "./" }, 
            { "Resources", "Resources" }
        }).Wait();
    }
}