using SB;
using SB.Core;

[TargetScript]
public static class CGltf
{
    static CGltf()
    {
        BuildSystem
            .Package("cgltf")
            .AddTarget("cgltf", (Target Target, PackageConfig Config) =>
            {
                if (Config.Version != new Version(1, 13, 0))
                    throw new TaskFatalError("cgltf version mismatch!", "cgltf version mismatch, only v1.13.0 is supported in source.");

                Target
                    .TargetType(TargetType.Static)
                    .RuntimeLibrary("MD")
                    .IncludeDirs(Visibility.Public, Path.Combine(SourceLocation.Directory(), "port/cgltf/include"))
                    .FpModel(FpModel.Fast)
                    .AddFiles("port/cgltf/src/cgltf.c");
            });
    }
}