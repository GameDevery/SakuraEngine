using SB;
using SB.Core;

[TargetScript]
public static class V8Playground
{
    static V8Playground()
    {
        Engine.Program("V8Playground")
            .Depend(Visibility.Public, "SkrV8")
            .IncludeDirs(Visibility.Public, "include")
            .AddCppFiles("src/**.cpp")
            .AddMetaHeaders("include/**.hpp")
            .DebugArgs("-r", Path.Join(SourceLocation.Directory(), "script/output"))
            // .DebugPreRunCmd($"dotnet run SB -- run V8Playground d -o {Path.Join(SourceLocation.Directory(), "script/types")}")
            .DebugPreRunCmd($"tsc -p {Path.Join(SourceLocation.Directory(), "script/tsconfig.json")}");
    }
}