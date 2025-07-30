using SB;
using SB.Core;

[TargetScript(TargetCategory.Package)]
public static class Lua
{
    static Lua()
    {
        BuildSystem
            .Package("lua")
            .AddTarget("lua", (Target Target, PackageConfig Config) =>
            {
                if (Config.Version != new Version(5, 4, 4))
                    throw new TaskFatalError("lua version mismatch!", "lua version mismatch, only v5.4.4 is supported in source.");

                Target
                    .CVersion("17")
                    .Exception(true)
                    .TargetType(TargetType.Static)
                    .IncludeDirs(Visibility.Public, "lua/include")
                    .IncludeDirs(Visibility.Private, "lua/include/lua")
                    .AddCFiles("lua/src/*.c");
            });
    }
}