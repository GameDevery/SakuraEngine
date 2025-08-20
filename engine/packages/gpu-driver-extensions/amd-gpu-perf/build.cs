using SB;
using SB.Core;

[TargetScript(TargetCategory.Package)]
public static class AmdGPUPerf
{
    static AmdGPUPerf()
    {
        // TODO: WE NEED TO ENABLE SETUP IN PACKAGE SCOPE
        BuildSystem.AddSetup<AmdGPUPerfSetup>();

        BuildSystem.Package("AmdGPUPerf")
            .AddTarget("AmdGPUPerf", (Target Target, PackageConfig Config) =>
            {
                Target.TargetType(TargetType.HeaderOnly);
                if (Config.Version == new Version(4, 1, 0))
                {
                    Target.IncludeDirs(Visibility.Public, "4.1.0.15");
                }
                else
                {
                    throw new TaskFatalError("AmdGPUPerf version mismatch!", "AmdGPUPerf version mismatch, only v4.1.0 is supported in source.");
                }
            });
    }
}

public class AmdGPUPerfSetup : ISetup
{
    public void Setup()
    {
        if (BuildSystem.TargetOS == OSPlatform.Windows)
        {
            Task.WaitAll(
                Install.SDK("amd-gpu-perf-4.1.0.15")
            );
        }
    }
}