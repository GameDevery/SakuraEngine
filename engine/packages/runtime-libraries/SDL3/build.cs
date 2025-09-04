using SB;
using SB.Core;

[TargetScript(TargetCategory.Package)]
public static class SDL3
{
    static SDL3()
    {
        BuildSystem.Package("SDL3")
            .AddTarget("SDL3", (Target Target, PackageConfig Config) =>
            {
                if (Config.Version != new Version(1, 0, 0))
                    throw new TaskFatalError("SDL3 version mismatch!", "SDL3 version mismatch, only v1.0.0 is supported in source.");

                Target.TargetType(TargetType.HeaderOnly)
                    .IncludeDirs(Visibility.Public, "include")
                    .Defines(Visibility.Public, "SDL_MAIN_HANDLED=1");
                
                if (BuildSystem.TargetOS == OSPlatform.Windows)
                {
                    Target.Link(Visibility.Public, "SDL3");
                }
                if (BuildSystem.TargetOS == OSPlatform.OSX)
                {
                    Target.Link(Visibility.Public, "SDL3.0");
                }
            });
    }
}