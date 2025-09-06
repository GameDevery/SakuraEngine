using SB;
using SB.Core;

[TargetScript(TargetCategory.Package)]
public static class AmdAgs
{
    static AmdAgs()
    {
        if (BuildSystem.TargetOS != OSPlatform.Windows)
            return;

        BuildSystem.Package("AmdAgs")
            .AddTarget("AmdAgs", (Target Target, PackageConfig Config) =>
            {
                BuildSystem.AddSetup<AmdAgsSetup>();

                Target.TargetType(TargetType.HeaderOnly);
                if (Config.Version == new Version(6, 3, 0))
                {
                    Target.IncludeDirs(Visibility.Public, "6.3.0");
                }
                else
                {
                    throw new TaskFatalError("AmdAgs version mismatch!", "AmdAgs version mismatch, only v6.3.0 is supported in source.");
                }
            });
    }
}

public class AmdAgsSetup : ISetup
{
    public void Setup()
    {
        if (BuildSystem.TargetOS == OSPlatform.Windows)
        {
            Task.WaitAll(
                Install.SDK("amdags-6.3.0")
            );
        }
    }
}