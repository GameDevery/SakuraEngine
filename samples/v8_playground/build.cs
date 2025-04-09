using SB;
using SB.Core;

[TargetScript]
public static class V8Playground
{
    static V8Playground()
    {
        Engine.Program("V8Playground")
            .Tags(TargetTags.V8)
            .Depend(Visibility.Public, "SkrV8")
            .IncludeDirs(Visibility.Public, "include")
            .AddCppFiles("src/**.cpp")
            .AddMetaHeaders("include/**.hpp");
    }
}