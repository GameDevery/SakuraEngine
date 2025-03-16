using SB.Core;
using System.Collections.Concurrent;
using System.Diagnostics;

namespace SB
{
    public class CodegenRenderEmitter : TaskEmitter
    {
        public CodegenRenderEmitter(IToolchain Toolchain)
        {
            this.Toolchain = Toolchain;
        }
        public override bool EnableEmitter(Target target) => true;
        public override bool EmitFileTask(Target Target) => false;
        public override bool FileFilter(Target Target, string File) => Target.GetTargetType() != TargetType.HeaderOnly && File.EndsWith(".cpp") || File.EndsWith(".c") || File.EndsWith(".cxx") || File.EndsWith(".cc");
        public override IArtifact? PerFileTask(Target Target, string SourceFile)
        {
            return null;
        }

        private IToolchain Toolchain { get; }
        public static volatile int Time = 0;
    }
}