using SB;
using SB.Core;

[TargetScript]
public static class SkrProfile
{
    static SkrProfile()
    {
        BuildSystem
            .Target("SkrProfile")
            .CppVersion("20")
            .TargetType(TargetType.Dynamic)
            .IncludeDirs(Visibility.Public, "include")
            .AddCppFiles("internal/tracy/TracyClient.cpp")
            .Defines(Visibility.Public, Engine.UseProfile ? "SKR_PROFILE_OVERRIDE_ENABLE" : "SKR_PROFILE_OVERRIDE_DISABLE")
            .ClangCl_CXFlags(Visibility.Private,
                "-Wno-missing-field-initializers", 
                "-Wno-format",
                "-Wno-unused-variable",
                "-Wno-unused-but-set-variable"
            ); // GCC, CLANG, CLANG_CL
    }
}