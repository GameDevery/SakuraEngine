using SB;
using SB.Core;

[TargetScript]
public static class SkrV8
{
    static SkrV8()
    {
        if (BuildSystem.TargetOS != OSPlatform.Windows)
            return;

        Engine.Module("SkrV8")
            .Depend(Visibility.Public, "v8", "libhv", "SkrRT")
            .IncludeDirs(Visibility.Public, "include")
            .AddCppFiles("src/**.cpp")
            .AddMetaHeaders("include/**.hpp");
    }
}