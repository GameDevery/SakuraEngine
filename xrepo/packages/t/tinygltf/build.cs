using SB;
using SB.Core;

[TargetScript]
public static class TinyGltf
{
    static TinyGltf()
    {
        BuildSystem
            .Package("tinygltf")
            .AddTarget("tinygltf", (Target Target, PackageConfig Config) =>
            {
                if (Config.Version != new Version(2, 8, 14))
                    throw new TaskFatalError("tinygltf version mismatch!", "tinygltf version mismatch, only v2.8.14 is supported in source.");

                Target
                    .CppVersion("20")
                    .RuntimeLibrary("MD")
                    .Exception(false)
                    .TargetType(TargetType.Static)
                    .IncludeDirs(Visibility.Public, Path.Combine(SourceLocation.Directory(), "port/src"))
                    .AddFiles("port/src/**.cc");
            });
    }
}