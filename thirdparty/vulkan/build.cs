using SB;
using SB.Core;

[TargetScript]
public static class VulkanHeaders
{
    static VulkanHeaders()
    {
        BuildSystem.Target("VulkanHeaders")
            .TargetType(TargetType.HeaderOnly)
            .IncludeDirs(Visibility.Public, "include");
    }
}