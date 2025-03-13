using SB.Core;
using System.Diagnostics;

namespace SB
{
    public class CppLinkEmitter : TaskEmitter
    {
        public CppLinkEmitter(IToolchain Toolchain) => this.Toolchain = Toolchain;
        public override bool EmitTargetTask => true;
        public override IArtifact PerTargetTask(Target Target)
        {
            var LinkedFileName = GetLinkedFileName(Target);
            var DependFile = Path.Combine(Target.GetStorePath(BuildSystem.DepsStore), BuildSystem.GetUniqueTempFileName(LinkedFileName, Target.Name + this.Name, "task.deps.json"));

            var Inputs = new ArgumentList<string>();
            // add obj files
            Inputs.AddRange(Target.AllFiles.Select(SourceFile => CppCompileEmitter.GetObjectFilePath(Target, SourceFile)));
            // add dep links
            if (Target.Arguments["TargetType"] is TargetType TT && TT != TargetType.Static)
            {
                Inputs.AddRange(Target.Dependencies.Select(T => GetLinkedFileName(BuildSystem.GetTarget(T))));
            }
            if (Inputs.Count != 0)
            {
                Stopwatch sw = new();
                sw.Start();
                var LINKDriver = (new LINKArgumentDriver() as IArgumentDriver)
                    .AddArguments(Target.Arguments)
                    .AddArgument("Inputs", Inputs)
                    .AddArgument("Output", LinkedFileName)
                    .AddArgument("DependFile", DependFile);
                var R = Toolchain.Linker.Link(LINKDriver);
                sw.Stop();
                Time += (int)sw.ElapsedMilliseconds;
                return R;
            }
            return null;
        }

        private static string GetLinkedFileName(Target Target)
        {
            var OutputType = (TargetType)Target.Arguments["TargetType"];
            var Extension = (OutputType == TargetType.Static) ? "lib" :
                            (OutputType == TargetType.Dynamic) ? "dll" :
                            (OutputType == TargetType.Executable) ? "exe" : "unknown";
            var OutputFile = Path.Combine(Target.GetBuildPath(), $"{Target.Name}.{Extension}");
            return OutputFile;
        }

        private IToolchain Toolchain { get; }
        public static volatile int Time = 0;
    }
}
