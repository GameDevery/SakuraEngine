using SB;
using SB.Core;

[TargetScript]
public static class Luau
{
    static Luau()
    {
        BuildSystem
            .Package("Luau")
            .AddTarget("Common", (Target Target, PackageConfig Config) =>
            {
                if (Config.Version != new Version(0, 613, 1))
                    throw new TaskFatalError("luau version mismatch!", "luau version mismatch, only v0.613.1 is supported in source.");

                Target
                    .CppVersion("17")
                    .Exception(false)
                    .TargetType(TargetType.HeaderOnly)
                    .IncludeDirs(Visibility.Public, Path.Combine(SourceLocation.Directory(), "port/luau/Common/include"));
            })
            .AddTarget("VM", (Target Target, PackageConfig Config) =>
            {
                if (Config.Version != new Version(0, 613, 1))
                    throw new TaskFatalError("luau version mismatch!", "luau version mismatch, only v0.613.1 is supported in source.");

                Target
                    .CppVersion("17")
                    .Exception(false)
                    .TargetType(TargetType.Static)
                    .Require("Luau", Config)
                    .Depend(Visibility.Public, "Luau@Common")
                    .Defines(Visibility.Public, "LUA_USE_LONGJMP=1", "LUA_API=extern\\\"C\\\"")
                    .IncludeDirs(Visibility.Public, Path.Combine(SourceLocation.Directory(), "port/luau/VM/include"))
                    .AddFiles("port/luau/VM/src/**.cpp")
                    .CppFlags(Visibility.Public, "-fno-math-errno");

                    if (BuildSystem.TargetOS == OSPlatform.Windows)
                    {
                        Target.AddFiles("port/luau/VM/src/lvmexecute.cc"); //, {force = {cxflags = "/d2ssa-pre-"}});
                    }
                    else
                    {
                        Target.AddFiles("port/luau/VM/src/lvmexecute.cc");
                    }
            })
            .AddTarget("Ast", (Target Target, PackageConfig Config) =>
            {
                if (Config.Version != new Version(0, 613, 1))
                    throw new TaskFatalError("luau version mismatch!", "luau version mismatch, only v0.613.1 is supported in source.");

                Target
                    .CppVersion("17")
                    .Exception(false)
                    .TargetType(TargetType.Static)
                    .Require("Luau", Config)
                    .Depend(Visibility.Public, "Luau@Common")
                    .IncludeDirs(Visibility.Public, Path.Combine(SourceLocation.Directory(), "port/luau/Ast/include"))
                    .AddFiles("port/luau/Ast/src/**.cpp");
            })
            .AddTarget("Compiler", (Target Target, PackageConfig Config) =>
            {
                if (Config.Version != new Version(0, 613, 1))
                    throw new TaskFatalError("luau version mismatch!", "luau version mismatch, only v0.613.1 is supported in source.");
                
                Target
                    .CppVersion("17")
                    .Exception(false)
                    .TargetType(TargetType.Static)
                    .Require("Luau", Config)
                    .Depend(Visibility.Public, "Luau@Ast")
                    .Defines(Visibility.Public, "LUACODE_API=extern\\\"C\\\"")
                    .IncludeDirs(Visibility.Public, Path.Combine(SourceLocation.Directory(), "port/luau/Compiler/include"))
                    .AddFiles("port/luau/Compiler/src/**.cpp");
            });
    }
}