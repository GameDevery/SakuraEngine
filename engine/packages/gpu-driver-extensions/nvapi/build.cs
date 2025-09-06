using SB;
using SB.Core;

[TargetScript(TargetCategory.Package)]
public static class NvApi
{
    static NvApi()
    {
        if (BuildSystem.TargetOS != OSPlatform.Windows)
            return;

        BuildSystem.Package("NvApi")
            .AddTarget("NvApi", (Target Target, PackageConfig Config) =>
            {
                BuildSystem.AddSetup<NvApiSetup>();

                Target.TargetType(TargetType.HeaderOnly);
                if (Config.Version == new Version(580, 0, 0))
                {
                    Target.IncludeDirs(Visibility.Public, "R580")
                        .Link(Visibility.Public, "nvapi64");
                }
                else
                {
                    throw new TaskFatalError("NvApi version mismatch!", "NvApi version mismatch, only v580.0.0 is supported in source.");
                }
            });
    }
}

public class NvApiSetup : ISetup
{
    public void Setup()
    {
        if (BuildSystem.TargetOS == OSPlatform.Windows)
        {
            Task.WaitAll(
                Install.SDK("nvapi-R580")
            );
        }
    }
}