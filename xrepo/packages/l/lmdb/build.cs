using SB;
using SB.Core;

[TargetScript]
public static class Lmdb
{
    static Lmdb()
    {
        BuildSystem
            .Package("lmdb")
            .AddTarget("lmdb", (Target Target, PackageConfig Config) =>
            {
                if (Config.Version != new Version(0, 9, 29))
                    throw new TaskFatalError("lmdb version mismatch!", "lmdb version mismatch, only v0.9.29 is supported in source.");

                Target
                    .CVersion("17")
                    .Exception(false)
                    .TargetType(TargetType.Static)
                    .IncludeDirs(Visibility.Public, Path.Combine(SourceLocation.Directory(), "port/lmdb/include"))
                    .IncludeDirs(Visibility.Private, Path.Combine(SourceLocation.Directory(), "port/lmdb/include/lmdb"))
                    .AddCFiles("port/lmdb/*.c");
            });
    }
}