using SB.Core;
using Serilog;

namespace SB
{
    using BS = BuildSystem;

    public class UnityBuildAttribute
    {

    }

    public class UnityBuildEmitter : TaskEmitter
    {
        public override bool EnableEmitter(Target Target) => Target.AllFiles.Any(F => F.Is_C_Cpp() || F.Is_OC_OCpp());
        public override bool EmitTargetTask(Target Target) => true;
        public override IArtifact? PerTargetTask(Target Target)
        {
            return null;
        }
    }

}