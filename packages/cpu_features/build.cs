using SB;
using SB.Core;

[TargetScript(TargetCategory.Package)]
public static class CPUFeatures
{
    static CPUFeatures()
    {
        BuildSystem.Package("cpu_features")
            .AddTarget("cpu_features", (Target Target, PackageConfig Config) =>
            {
                if (Config.Version != new Version(0, 9, 0))
                    throw new TaskFatalError("cpu_features version mismatch!", "cpu_features version mismatch, only v0.9.0 is supported in source.");

                Target.TargetType(TargetType.Static)
                    .IncludeDirs(Visibility.Public, "include")
                    .IncludeDirs(Visibility.Private, "include/cpu_features")
                    .FpModel(FpModel.Fast)
                    .AddCFiles("build.cpu_features.c");
            });
    }
}