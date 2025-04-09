using SB;
using SB.Core;
using Serilog;

[TargetScript]
[CefDoctor]
public static class Cef
{
    static Cef()
    {
        var @this = Engine.Target("cef")
            .TargetType(TargetType.Static)
            .Defines(Visibility.Public, "WRAPPING_CEF_SHARED", "NOMINMAX", "USING_CEF_SHARED=1")
            .Defines(Visibility.Private, "_HAS_EXCEPTIONS=0")
            .IncludeDirs(Visibility.Public, "include")
            .IncludeDirs(Visibility.Private, "./")
            .Link(Visibility.Private, "libcef")
            .AddCppFiles("libcef_dll/**.cc");

        @this.CXFlags_ClangCl(Visibility.Private, "-Wno-undefined-var-template");
    }
}

public class CefDoctor : DoctorAttribute
{
    public override bool Check()
    {
        Install.SDK("cef-6778", new Dictionary<string, string> {
            { "Release", "./" }, 
            { "Resources", "Resources" }
        }).Wait();
        return true;
    }

    public override bool Fix()
    {
        Log.Fatal("Cef SDK install failed!");
        return true;
    }
}