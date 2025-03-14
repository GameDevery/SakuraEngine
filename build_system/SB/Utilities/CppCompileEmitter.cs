using SB.Core;
using System.Diagnostics;

namespace SB
{
    public class CppCompileEmitter : TaskEmitter
    {
        public CppCompileEmitter(IToolchain Toolchain)
        {
            this.Toolchain = Toolchain;
        }
        public override bool EmitFileTask => true;

        public override bool FileFilter(string File) => true;

        public override IArtifact PerFileTask(Target Target, string SourceFile)
        {
            Stopwatch sw = new();
            sw.Start();

            var SourceDependencies = Path.Combine(Target.GetStorePath(BuildSystem.DepsStore), BuildSystem.GetUniqueTempFileName(SourceFile, Target.Name + this.Name, "source.deps.json"));
            var ObjectFile = GetObjectFilePath(Target, SourceFile);

            var CLDriver = (new CLArgumentDriver() as IArgumentDriver)
                .AddArguments(Target.Arguments)
                .AddArgument("Source", SourceFile)
                .AddArgument("Object", ObjectFile)
                .AddArgument("SourceDependencies", SourceDependencies);

            var R = Toolchain.Compiler.Compile(Target.Name, Name, CLDriver);
            sw.Stop();
            Time += (int)sw.ElapsedMilliseconds;
            return R;
        }

        public static string GetObjectFilePath(Target Target, string SourceFile) => Path.Combine(Target.GetStorePath(BuildSystem.ObjsStore), BuildSystem.GetUniqueTempFileName(SourceFile, Target.Name, "obj"));

        private IToolchain Toolchain { get; }
        public static volatile int Time = 0;
    }
}