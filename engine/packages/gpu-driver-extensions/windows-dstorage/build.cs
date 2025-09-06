using SB;
using SB.Core;

[TargetScript(TargetCategory.Package)]
public static class DirectStorage
{
    static DirectStorage()
    {
        if (BuildSystem.TargetOS != OSPlatform.Windows)
            return;

        BuildSystem.Package("DirectStorage")
            .AddTarget("DirectStorage", (Target Target, PackageConfig Config) =>
            {
                BuildSystem.AddSetup<DirectStorageSetup>();

                Target.TargetType(TargetType.HeaderOnly);
                if (Config.Version == new Version(1, 3, 0))
                {
                    Target.IncludeDirs(Visibility.Public, "1.3.0");
                }
                else
                {
                    throw new TaskFatalError("DirectStorage version mismatch!", "DirectStorage version mismatch, only v1.3.0 is supported in source.");
                }
            });
    }
}

public class DirectStorageSetup : ISetup
{
    public void Setup()
    {
        if (BuildSystem.TargetOS == OSPlatform.Windows)
        {
            Task.WaitAll(
                Install.SDK("dstorage-1.3.0")
            );
        }
    }
}