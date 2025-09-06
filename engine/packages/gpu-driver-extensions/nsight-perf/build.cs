using SB;
using SB.Core;

[TargetScript(TargetCategory.Package)]
public static class NvPerf
{
    static NvPerf()
    {
        if (BuildSystem.TargetOS != OSPlatform.Windows)
            return;

        BuildSystem.Package("NvPerf")
            .AddTarget("NvPerf", (Target Target, PackageConfig Config) =>
            {
                BuildSystem.AddSetup<NvPerfSetUp>();

                Target.TargetType(TargetType.HeaderOnly);
                if (Config.Version == new Version(2025, 1, 0))
                {
                    Target.IncludeDirs(Visibility.Public, "2025.1")
                        .Link(Visibility.Public, "nvperf_grfx_host");
                }
                else
                {
                    throw new TaskFatalError("NvPerf version mismatch!", "NvPerf version mismatch, only v2025.1.0 is supported in source.");
                }
            });
    }
}

public class NvPerfSetUp : ISetup
{
    public void Setup()
    {
        if (BuildSystem.TargetOS == OSPlatform.Windows)
        {
            Task.WaitAll(
                Install.SDK("nvperf-2025.1")
            );
        }
    }
}