using SB;
using SB.Core;

[TargetScript(TargetCategory.Package)]
public static class Lemon
{
    static Lemon()
    {
        BuildSystem.Package("lemon")
            .AddTarget("lemon", (Target Target, PackageConfig Config) =>
            {
                if (Config.Version != new Version(1, 3, 1))
                    throw new TaskFatalError("lemon version mismatch!", "lemon version mismatch, only v1.3.1 is supported in source.");

                Target.CppVersion("20")
                    .Exception(true)
                    .TargetType(TargetType.Static)
                    .IncludeDirs(Visibility.Public, "./")
                    .AddCppFiles(
                        "lemon/arg_parser.cc",
                        "lemon/base.cc",
                        "lemon/color.cc",
                        "lemon/lp_base.cc",
                        "lemon/lp_skeleton.cc",
                        "lemon/random.cc",
                        "lemon/bits/windows.cc"
                    );

                if (BuildSystem.TargetOS == OSPlatform.Windows)
                {
                    Target.Defines(Visibility.Private, "WIN32")
                        .Link(Visibility.Private, "ole32");
                }
                else
                {
                    Target.Link(Visibility.Private, "pthread");
                }
            });
    }
}