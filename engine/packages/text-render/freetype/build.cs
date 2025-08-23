using SB;
using SB.Core;

[TargetScript(TargetCategory.Package)]
public static class Freetype
{
    static Freetype()
    {
        BuildSystem.Package("freetype")
            .AddTarget("freetype", (Target Target, PackageConfig Config) =>
            {
                if (Config.Version != new Version(2, 13, 0))
                    throw new TaskFatalError("freetype version mismatch!", "freetype version mismatch, only v2.13.0 is supported in source.");

                Target.TargetType(TargetType.Static)
                    .EnableUnityBuild()
                    .OptimizationLevel(OptimizationLevel.Fastest)
                    .IncludeDirs(Visibility.Public, "freetype/include")
                    .IncludeDirs(Visibility.Private, "ft-zlib")
                    .Defines(Visibility.Private, "FT2_BUILD_LIBRARY", "FT_CONFIG_OPTION_SYSTEM_ZLIB")
                    .Require("zlib", new PackageConfig { Version = new(1, 2, 8) })
                    .Depend(Visibility.Public, "zlib@zlib")
                    .ClangCl_CFlags(Visibility.Private, "-Wno-macro-redefined")
                    .Cl_CFlags(Visibility.Private, "/wd4005")
                    .AddCFiles(
                        "freetype/src/autofit/autofit.c",
                        "freetype/src/base/ftbase.c",
                        "freetype/src/base/ftbbox.c",
                        "freetype/src/base/ftbdf.c",
                        "freetype/src/base/ftbitmap.c",
                        "freetype/src/base/ftcid.c",
                        "freetype/src/base/ftfstype.c",
                        "freetype/src/base/ftgasp.c",
                        "freetype/src/base/ftglyph.c",
                        "freetype/src/base/ftgxval.c",
                        "freetype/src/base/ftinit.c",
                        "freetype/src/base/ftmm.c",
                        "freetype/src/base/ftotval.c",
                        "freetype/src/base/ftpatent.c",
                        "freetype/src/base/ftpfr.c",
                        "freetype/src/base/ftstroke.c",
                        "freetype/src/base/ftsynth.c",
                        "freetype/src/base/fttype1.c",
                        "freetype/src/base/ftwinfnt.c",
                        "freetype/src/bdf/bdf.c",
                        "freetype/src/bzip2/ftbzip2.c",
                        "freetype/src/cache/ftcache.c",
                        "freetype/src/cff/cff.c",
                        "freetype/src/cid/type1cid.c",
                        "freetype/src/gzip/ftgzip.c",
                        "freetype/src/lzw/ftlzw.c",
                        "freetype/src/pcf/pcf.c",
                        "freetype/src/pfr/pfr.c",
                        "freetype/src/psaux/psaux.c",
                        "freetype/src/pshinter/pshinter.c",
                        "freetype/src/psnames/psnames.c",
                        "freetype/src/raster/raster.c",
                        "freetype/src/sdf/sdf.c",
                        "freetype/src/sfnt/sfnt.c",
                        "freetype/src/smooth/smooth.c",
                        "freetype/src/svg/svg.c",
                        "freetype/src/truetype/truetype.c",
                        "freetype/src/type1/type1.c",
                        "freetype/src/type42/type42.c",
                        "freetype/src/winfonts/winfnt.c"
                    );

                if (BuildSystem.TargetOS == OSPlatform.Windows)
                {
                    Target
                        .AddCFiles("freetype/builds/windows/ftsystem.c", "freetype/builds/windows/ftdebug.c")
                        .CXFlags(Visibility.Private, "/wd4267", "/wd4244", "/utf-8")
                        .Defines(Visibility.Private, "_CRT_SECURE_NO_WARNINGS");
                }
                else
                {
                    Target
                        .AddCFiles("freetype/src/base/ftsystem.c", "freetype/src/base/ftdebug.c");
                }
            });
    }
}