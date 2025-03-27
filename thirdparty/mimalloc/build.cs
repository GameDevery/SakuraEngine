using SB;
using SB.Core;

[TargetScript]
public static class MiMalloc
{
    static MiMalloc()
    {
        var MiMalloc = Engine.Target("mimalloc")
            .TargetType(TargetType.Dynamic)
            .OptimizationLevel(OptimizationLevel.Fastest)
            .CppVersion("20")
            .Defines(Visibility.Public, "MI_SHARED_LIB")
            .Defines(Visibility.Private, "MI_SHARED_LIB_EXPORT", "MI_XMALLOC=1", "MI_WIN_NOREDIRECT")
            .IncludeDirs(Visibility.Public, "files")
            .AddCppFiles("files/build.mimalloc.cpp");
        
        if (BuildSystem.TargetOS == OSPlatform.Windows)
        {
            MiMalloc.Link(Visibility.Private, "psapi", "shell32", "user32", "advapi32", "bcrypt");
            MiMalloc.Defines(Visibility.Private, "_CRT_SECURE_NO_WARNINGS");
        }
    }
}