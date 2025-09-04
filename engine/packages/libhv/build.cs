using SB;
using SB.Core;

[TargetScript(TargetCategory.Package)]
public static class LibHV
{
    static LibHV()
    {
        Engine.AddSetup<LibHVSetup>();

        BuildSystem.Package("LibHV")
            .AddTarget("LibHV", (Target Target, PackageConfig Config) =>
            {
                if (Config.Version != new Version(1, 3, 3))
                    throw new TaskFatalError("LibHV version mismatch!", "LibHV version mismatch, only v1.3.3 is supported in source.");

                Target.TargetType(TargetType.HeaderOnly)
                    .Defines(Visibility.Public, "HV_STATICLIB")
                    .IncludeDirs(Visibility.Public, "include")
                    .Link(Visibility.Public, "hv_static")
                    .AppleFramework(Visibility.Public, "Security")
                    .Clang_CXFlags(Visibility.Public, "-Wno-deprecated-literal-operator");
            });
    }
}

public class LibHVSetup : ISetup
{
    public void Setup()
    {
        Install.SDK("libhv_1.3.3a").Wait();
    }
}