using SB;
using SB.Core;

[TargetScript]
public static class MiMalloc
{
    static MiMalloc()
    {
        var MiMalloc = Engine.Target("mimalloc")
            .OptimizationLevel(OptimizationLevel.Fastest)
            .CVersion("11")
            .Defines(Visibility.Private, "MI_WIN_NOREDIRECT")
            .IncludeDirs(Visibility.Public, "files")
            .AddCFiles("files/build.mimalloc.c");

        if (!Engine.ShippingOneArchive)
        {
            MiMalloc.TargetType(TargetType.Dynamic)
                .Defines(Visibility.Public, "MI_SHARED_LIB")
                .Defines(Visibility.Private, "MI_SHARED_LIB_EXPORT");
        }
        else
        {
            MiMalloc.TargetType(TargetType.Objects);
        }

        if (BuildSystem.TargetOS == OSPlatform.Windows)
        {
            MiMalloc.Link(Visibility.Private, "psapi", "shell32", "ole32", "advapi32", "bcrypt");
            MiMalloc.Defines(Visibility.Private, "_CRT_SECURE_NO_WARNINGS");
        }
    }
}