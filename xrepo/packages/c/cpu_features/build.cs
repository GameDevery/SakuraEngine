using SB;
using SB.Core;

[TargetScript]
public static class CPUFeatures
{
    static CPUFeatures()
    {
        BuildSystem
            .Package("cpu_features")
            .AddTarget("cpu_features", (Target Target, PackageConfig Config) =>
            {
                if (Config.Version != new Version(0, 9, 0))
                    throw new TaskFatalError("cpu_features version mismatch!", "cpu_features version mismatch, only v0.9.0 is supported in source.");

                Target
                    .TargetType(TargetType.Static)
                    .IncludeDirs(Visibility.Public, Path.Combine(SourceLocation.Directory(), "port/cpu_features/include"))
                    .IncludeDirs(Visibility.Private, Path.Combine(SourceLocation.Directory(), "port/cpu_features/include/cpu_features"))
                    .FpModel(FpModel.Fast)
                    .AddFiles("port/cpu_features/build.cpu_features.c");
            });
    }
}