using SB;
using SB.Core;

[TargetScript]
public static class SceneSamples
{
    static SceneSamples()
    {
        Engine.Program("SceneSample_Serde")
            .IncludeDirs(Visibility.Private, ".")
            .AddCppFiles("serde/*.cpp")
            .Depend(Visibility.Private, "SkrScene")
            .Depend(Visibility.Private, "SkrCore");
        Engine.Program("SceneSample_Simple")
            .IncludeDirs(Visibility.Private, ".")
            .AddCppFiles("simple/*.cpp")
            .Depend(Visibility.Private, "SkrScene")
            .Depend(Visibility.Private, "SkrCore");
    }
}