using SB.Core;

namespace SB
{
    public static partial class BuildSystem
    {
        public static Architecture HostArch => HostInformation.HostArch;
        public static OSPlatform HostOS => HostInformation.HostOS;
        public static Architecture TargetArch = HostArch;
        public static OSPlatform TargetOS = HostOS;
    }
}
