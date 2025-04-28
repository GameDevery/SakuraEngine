using SB;
using SB.Core;
using Serilog;

[TargetScript]
public static class SketchUpDump
{
    static SketchUpDump()
    {
        Engine.Program("SketchUpDump")
            .EnableUnityBuild()
            .Depend(Visibility.Private, "SkrCore", "SkrTask", "SkrSketchUp")
            .IncludeDirs(Visibility.Private, "include")
            .AddCppFiles("src/**.cpp");
    }
}
