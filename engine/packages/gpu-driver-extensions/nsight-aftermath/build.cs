using SB;
using SB.Core;

[TargetScript(TargetCategory.Package)]
public static class NvAftermath
{
    static NvAftermath()
    {
        // TODO: WE NEED TO ENABLE SETUP IN PACKAGE SCOPE
        BuildSystem.AddSetup<NvAftermathSetup>();

        BuildSystem.Package("NvAftermath")
            .AddTarget("NvAftermath", (Target Target, PackageConfig Config) =>
            {
                Target.TargetType(TargetType.HeaderOnly);
                if (Config.Version == new Version(2025, 1, 0))
                {
                    Target.IncludeDirs(Visibility.Public, "2025.1.0.25009");
                }
                else
                {
                    throw new TaskFatalError("NvAftermath version mismatch!", "NvAftermath version mismatch, only v2025.1.0 is supported in source.");
                }
            });
    }
}

public class NvAftermathSetup : ISetup
{
    public void Setup()
    {
        if (BuildSystem.TargetOS == OSPlatform.Windows)
        {
            Task.WaitAll(
                Install.SDK("nsight-2025.1.0.25009")
            );
        }
    }
}