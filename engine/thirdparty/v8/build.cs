using SB;
using SB.Core;
using Serilog;

[TargetScript]
public static class V8
{
    static V8()
    {
        Engine.AddSetup<V8Setup>();

        var V8 = BuildSystem.Target("v8")
            .TargetType(TargetType.HeaderOnly)
            .Depend(Visibility.Public, "SkrRT")
            .IncludeDirs(Visibility.Public, "include")
            .Defines(Visibility.Public, "USING_V8_PLATFORM_SHARED", "USING_V8_SHARED");
        
        if (BuildSystem.TargetOS == OSPlatform.Windows)
            V8.Link(Visibility.Public, "v8.dll", "v8_libplatform.dll");
        else if (BuildSystem.TargetOS == OSPlatform.OSX)
            V8.Link(Visibility.Public, "v8", "v8_libbase", "v8_libplatform", "chrome_zlib", "third_party_abseil-cpp_absl");
    }
}

public class V8Setup : ISetup
{
    public void Setup()
    {
        Install.SDK("v8_11.8.172").Wait();
    }
}