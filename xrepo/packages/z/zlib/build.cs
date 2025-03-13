using SB;
using SB.Core;

[TargetScript]
public static class Zlib
{
    static Zlib()
    {
        BuildSystem
            .Package("zlib")
            .AvailableVersions(new Version(1, 2, 8))
            .AddTarget("zlib", (Target Target, PackageConfig Config) =>
            {
                if (Config.Version != new Version(1, 2, 8))
                    throw new TaskFatalError("zlib version mismatch!", "zlib version mismatch, only v1.2.8 is supported in source.");

                Target
                    .TargetType(TargetType.Static)
                    .IncludeDirs(Visibility.Public, Path.Combine(SourceLocation.Directory(), "port/zlib/include"));
                
                if (BuildSystem.TargetOS == OSPlatform.Windows)
                {
                    Target
                        .LinkDirs(Visibility.Public, Path.Combine(SourceLocation.Directory(), "port/zlib/lib/windows/x64"))
                        .Link(Visibility.Public, "zlibstatic.lib");
                }
                else if (BuildSystem.TargetOS == OSPlatform.OSX)
                {
                    Target
                        .LinkDirs(Visibility.Public, Path.Combine(SourceLocation.Directory(), "port/zlib/lib/macos/x86_64"))
                        .Link(Visibility.Public, "libz.a");
                }
                else
                {
                    throw new TaskFatalError("Unsupported OS", "zlib is not supported on this OS.");
                }
            });
    }
} 