using SB;
using SB.Core;

[TargetScript]
public static class SkrTask
{
    static SkrTask()
    {
        Engine.Module("SkrTask")
            .Require("Marl", new PackageConfig { Version = new Version(1, 0, 0) })
            .Depend(Visibility.Public, "Marl@Marl")
            .IncludeDirs(Visibility.Public, "include")
            .AddCppFiles("src/build.*.cpp")
            .Depend(Visibility.Public, "SkrCore");
    }
}