using SB;
using SB.Core;

[TargetScript]
public static class YYJson
{
    static YYJson()
    {
        BuildSystem
            .Package("yyjson")
            .AddTarget("yyjson", (Target Target, PackageConfig Config) =>
            {
                if (Config.Version != new Version(0, 9, 0))
                    throw new TaskFatalError("YYJson version mismatch!", "YYJson version mismatch, only v0.9.0 is supported in source.");

                Target
                    .TargetType(TargetType.Static)
                    .RuntimeLibrary("MD")
                    .IncludeDirs(Visibility.Private, Path.Combine(SourceLocation.Directory(), "port/yyjson"))
                    .IncludeDirs(Visibility.Public, Path.Combine(SourceLocation.Directory(), "port"))
                    .FpModel(FpModel.Fast)
                    .AddFiles("port/yyjson/**.c");
            });
    }
}