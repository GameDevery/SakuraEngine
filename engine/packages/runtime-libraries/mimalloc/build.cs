using SB;
using SB.Core;

[TargetScript(TargetCategory.Package)]
public static class MiMalloc
{
    static MiMalloc()
    {
        BuildSystem.Package("MiMalloc")
            .AddTarget("MiMalloc", (Target Target, PackageConfig Config) =>
            {
                if (Config.Version != new Version(3, 1, 5))
                    throw new TaskFatalError("mimalloc version mismatch!", "mimalloc version mismatch, only v3.1.5 is supported in source.");

                Target.OptimizationLevel(OptimizationLevel.Fastest)
                    .CVersion("11")
                    .Defines(Visibility.Private, "MI_WIN_NOREDIRECT")
                    .IncludeDirs(Visibility.Public, "files")
                    .AddCFiles("files/build.mimalloc.c");

                if (!Engine.ShippingOneArchive)
                {
                    Target.TargetType(TargetType.Dynamic)
                        .Defines(Visibility.Public, "MI_SHARED_LIB")
                        .Defines(Visibility.Private, "MI_SHARED_LIB_EXPORT");
                }
                else
                {
                    Target.TargetType(TargetType.Objects);
                }

                if (BuildSystem.TargetOS == OSPlatform.Windows)
                {
                    Target.Link(Visibility.Private, "psapi", "shell32", "ole32", "advapi32", "bcrypt");
                    Target.Defines(Visibility.Private, "_CRT_SECURE_NO_WARNINGS");
                }
            });
    }
}