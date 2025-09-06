using SB;
using SB.Core;

[TargetScript(TargetCategory.Package)]
public static class WinPixEventRuntime
{
    static WinPixEventRuntime()
    {
        BuildSystem.Package("WinPixEventRuntime")
            .AddTarget("WinPixEventRuntime", (Target Target, PackageConfig Config) =>
            {
                BuildSystem.AddSetup<WinPixEventRuntimeSetup>();

                Target.TargetType(TargetType.HeaderOnly);
                if (Config.Version == new Version(1, 0, 240308001))
                {
                    Target.IncludeDirs(Visibility.Public, "1.0.240308001");
                }
                else
                {
                    throw new TaskFatalError("WinPixEventRuntimeSetup version mismatch!", "WinPixEventRuntimeSetup version mismatch, only v1.0.240308001 is supported in source.");
                }
            });
    }
}

public class WinPixEventRuntimeSetup : ISetup
{
    public void Setup()
    {
        if (BuildSystem.TargetOS == OSPlatform.Windows)
        {
            Task.WaitAll(
                Install.SDK("WinPixEventRuntime-1.0.240308001")
            );
        }
    }
}