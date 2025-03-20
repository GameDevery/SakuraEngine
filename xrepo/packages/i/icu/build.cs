using SB;
using SB.Core;

[TargetScript]
public static class ICU
{
    static ICU()
    {
        BuildSystem
            .Package("icu")
            .AddTarget("icu", (Target Target, PackageConfig Config) =>
            {
                if (Config.Version != new Version(72, 1, 0))
                    throw new TaskFatalError("icu version mismatch!", "icu version mismatch, only v72.1.0 is supported in source.");

                Target
                    .TargetType(TargetType.Static)
                    .CppVersion("20")
                    .IncludeDirs(Visibility.Public, Path.Combine(SourceLocation.Directory(), "port/icu4c/source/common"))
                    .IncludeDirs(Visibility.Public, Path.Combine(SourceLocation.Directory(), "port/icu4c/source/i18n"))
                    .Defines(Visibility.Private, "U_I18N_IMPLEMENTATION")
                    .Defines(Visibility.Private, "U_COMMON_IMPLEMENTATION")
                    .Defines(Visibility.Private, "U_STATIC_IMPLEMENTATION")
                    .AddFiles(
                        "port/icu4c/source/i18n/**.cpp",
                        "port/icu4c/source/common/**.cpp",
                        "port/icu4c/source/stubdata/**.cpp"
                    );
                /*
                if (is_plat("windows")) then
                    add_cxflags("/wd4267", "/wd4244", "/source-charset:utf-8", {public=false})
                end
                */
            });
    }
}