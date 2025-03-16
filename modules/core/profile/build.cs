using SB;
using SB.Core;

[TargetScript]
public static class SkrProfile
{
    static SkrProfile()
    {
        BuildSystem
            .Target("SkrProfile")
            .TargetType(TargetType.Dynamic)
            .IncludeDirs(Visibility.Public, Path.Combine(SourceLocation.Directory(), "include"))
            .AddFiles("internal/tracy/TracyClient.cpp")
            .Defines(Visibility.Public, Engine.UseProfile ? "SKR_PROFILE_OVERRIDE_ENABLE" : "SKR_PROFILE_OVERRIDE_DISABLE");
            /*
            .CppFlags(
                "-Wno-missing-field-initializers", 
                "-Wno-format",
                "-Wno-unused-variable",
                "-Wno-unused-but-set-variable"
            ) // GCC, CLANG, CLANG_CL
            */
    }
}