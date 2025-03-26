using SB;
using SB.Core;

[TargetScript]
public static class SDL2
{
    static SDL2()
    {
        var SDL2 = Engine.Target("SDL2");
        SDL2.TargetType(TargetType.HeaderOnly)
            .LinkDirs(Visibility.Public, SDL2.GetBinaryPath())
            .IncludeDirs(Visibility.Public, Path.Combine(SourceLocation.Directory(), "include"));

        if (BuildSystem.TargetOS == OSPlatform.Windows)
            SDL2.Link(Visibility.Public, "SDL2");
        else if (BuildSystem.TargetOS == OSPlatform.OSX)
            SDL2.Link(Visibility.Public, "SDL2-2.0.0");
    }
}