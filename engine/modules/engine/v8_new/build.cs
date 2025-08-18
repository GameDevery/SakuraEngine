using SB;
using SB.Core;

[TargetScript]
public static class SkrV8New
{
    static SkrV8New()
    {
        Engine.Module("SkrV8New")
            .Depend(Visibility.Public, "v8", "libhv", "SkrRT")
            .IncludeDirs(Visibility.Public, "include")
            .AddCppFiles("src/**.cpp")
            .AddMetaHeaders("include/**.hpp");
    }
}