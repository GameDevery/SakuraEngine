using SB;
using SB.Core;

[TargetScript(TargetCategory.Package)]
public static class AmdGPUPerf
{
    static AmdGPUPerf()
    {
        if (BuildSystem.TargetOS != OSPlatform.Windows)
            return;

        BuildSystem.Package("AmdGPUPerf")
            .AddTarget("AmdGPUPerf", (Target Target, PackageConfig Config) =>
            {
                BuildSystem.AddSetup<AmdGPUPerfSetup>();

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