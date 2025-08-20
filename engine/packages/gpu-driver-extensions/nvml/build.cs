using SB;
using SB.Core;

[TargetScript(TargetCategory.Package)]
public static class NvML
{
    static NvML()
    {
        BuildSystem.Package("NvML")
            .AddTarget("NvML", (Target Target, PackageConfig Config) =>
            {
                Target.TargetType(TargetType.HeaderOnly);
                if (Config.Version == new Version(13, 0, 0))
                {
                    Target.IncludeDirs(Visibility.Public, "13.0.0")
                        .LinkDirs(Visibility.Public, "13.0.0")
                        .Link(Visibility.Public, "nvml");
                }
                else
                {
                    throw new TaskFatalError("NvML version mismatch!", "NvML version mismatch, only v13.0.0 is supported in source.");
                }
            });
    }
}