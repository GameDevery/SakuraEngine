using SB;
using SB.Core;

[TargetScript(TargetCategory.Package)]
public static class VulkanHeaders
{
    static VulkanHeaders()
    {
        BuildSystem.Package("VulkanHeaders")
            .AddTarget("VulkanHeaders", (Target Target, PackageConfig Config) =>
            {
                Target.TargetType(TargetType.HeaderOnly);
                if (Config.Version == new Version(1, 4, 304))
                {
                    Target.IncludeDirs(Visibility.Public, "1.4.304");
                }
                else if (Config.Version == new Version(1, 4, 324))
                {
                    Target.IncludeDirs(Visibility.Public, "1.4.324");
                }
                else
                {
                    throw new TaskFatalError("VulkanHeaders version mismatch!", "VulkanHeaders version mismatch, only v1.4.304/v1.4.324 is supported in source.");
                }
            });
    }
}