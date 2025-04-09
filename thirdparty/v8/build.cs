using SB;
using SB.Core;
using Serilog;

[TargetScript]
[V8Doctor]
public static class V8
{
    static V8()
    {
        BuildSystem.Target("v8")
            .TargetType(TargetType.HeaderOnly)
            .Depend(Visibility.Public, "SkrRT")
            .IncludeDirs(Visibility.Public, "include")
            .Defines(Visibility.Public, "USING_V8_PLATFORM_SHARED", "USING_V8_SHARED")
            .Link(Visibility.Public, "v8.dll", "v8_libbase.dll", "v8_libplatform.dll", "third_party_zlib.dll");
    }
}

public class V8Doctor : DoctorAttribute
{
    public override bool Check()
    {
        Install.SDK("v8_11.2_msvc", new Dictionary<string, string> {
            { "include", Path.Combine(SourceLocation.Directory(), "include") }, 
            { "bin", "./" }, 
            { "lib", "./" }
        }).Wait();
        return true;
    }

    public override bool Fix()
    {
        Log.Fatal("Cef SDK install failed!");
        return true;
    }   
}