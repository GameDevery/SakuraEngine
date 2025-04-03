
using SB.Core;

namespace SB
{
    public partial class BuildSystem
    {
        public static void LoadConfigurations()
        {
            TargetDelegate Debug = (Target) =>
            {
                Target.DebugSymbols(true)
                    .OptimizationLevel(OptimizationLevel.None);
            };
            Configurations.Add("debug", Debug);

            TargetDelegate DebugRelease = (Target) =>
            {
                Target.DebugSymbols(true)
                    .OptimizationLevel(OptimizationLevel.Faster);
            };
            Configurations.Add("releasedbg", DebugRelease);

            TargetDelegate Release = (Target) =>
            {
                Target.DebugSymbols(false)
                    .OptimizationLevel(OptimizationLevel.Fastest);
            };
            Configurations.Add("release", Release);
        }

        public static Dictionary<string, TargetDelegate> Configurations = new();
        public static string GlobalConfiguration = "debug";
    }
}