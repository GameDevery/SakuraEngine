using SB;
using SB.Core;

[TargetScript(TargetCategory.Package)]
public static class RTM
{
    static RTM()
    {
        BuildSystem.Package("RTM")
            .AddTarget("RTM", (Target Target, PackageConfig Config) =>
            {
                if (Config.Version != new Version(2, 3, 1))
                    throw new TaskFatalError("rtm version mismatch!", "rtm version mismatch, only v1.0.0 is supported in source.");

                Target.TargetType(TargetType.HeaderOnly)
                    .IncludeDirs(Visibility.Public, "include")
                    .Defines(Visibility.Public, "RTM_ON_ASSERT_ABORT")
                    .AddNatvisFiles("rtm.natvis");
            });
    }
}