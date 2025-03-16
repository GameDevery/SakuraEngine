using SB.Core;
using System.Diagnostics;

namespace SB
{
    public class CppLinkEmitter : TaskEmitter
    {
        public CppLinkEmitter(IToolchain Toolchain) => this.Toolchain = Toolchain;
        public override bool EmitTargetTask => true;
        public override IArtifact? PerTargetTask(Target Target)
        {
            var TT = Target.GetTargetType();
            if (TT != TargetType.HeaderOnly && TT != TargetType.Objects)
            {
                var LinkedFileName = GetLinkedFileName(Target);
                var DependFile = Path.Combine(Target.GetStorePath(BuildSystem.DepsStore), BuildSystem.GetUniqueTempFileName(LinkedFileName, Target.Name + this.Name, "task.deps.json"));
                var Inputs = new ArgumentList<string>();
                // Add obj files
                Inputs.AddRange(Target.AllFiles.Select(SourceFile => CppCompileEmitter.GetObjectFilePath(Target, SourceFile)));
                // Add dep obj files
                Inputs.AddRange(Target.Dependencies.Where(
                    Dep => BuildSystem.GetTarget(Dep)?.GetTargetType() == TargetType.Objects
                ).SelectMany(
                    Dep => BuildSystem.GetTarget(Dep).GetAttribute<CppCompileAttribute>()!.ObjectFiles
                ));
                // Add dep links.Static libraries don’t really have a link phase. 
                // Very cruedly they are just an archive of object files that will be propagated to a real link line ( creation of a shared library or executable ).
                if (TT != TargetType.Static)
                {
                    // add libs to link
                    Inputs.AddRange(
                        Target.Dependencies.Select(Dependency => GetStubFileName(BuildSystem.GetTarget(Dependency))).Where(Stub => Stub != null).Select(Stub => Stub!)
                    );
                }
                if (Inputs.Count != 0)
                {
                    Stopwatch sw = new();
                    sw.Start();

                    var LINKDriver = (new LINKArgumentDriver() as IArgumentDriver)
                        .AddArguments(Target.Arguments)
                        .AddArgument("Inputs", Inputs)
                        .AddArgument("Output", LinkedFileName);
                    var R = Toolchain.Linker.Link(this, Target, LINKDriver);
                    
                    sw.Stop();
                    Time += (int)sw.ElapsedMilliseconds;
                    return R;
                }
            }
            return null;
        }

        private static string GetLinkedFileName(Target Target)
        {
            var OutputType = Target.GetTargetType();
            var Extension = (OutputType == TargetType.Static) ? "lib" :
                            (OutputType == TargetType.Dynamic) ? "dll" :
                            (OutputType == TargetType.Executable) ? "exe" : "unknown";
            var OutputFile = Path.Combine(Target.GetBuildPath(), $"{Target.Name}.{Extension}");
            return OutputFile;
        }

        private static string? GetStubFileName(Target Target)
        {
            var OutputType = Target.GetTargetType();
            var Extension = (OutputType == TargetType.Static) ? "lib" :
                            (OutputType == TargetType.Dynamic) ? "lib" : "";
            if (Extension.Length == 0)
                return null;
            var OutputFile = Path.Combine(Target.GetBuildPath(), $"{Target.Name}.{Extension}");
            return OutputFile;
        }

        private IToolchain Toolchain { get; }
        public static volatile int Time = 0;
    }
}
