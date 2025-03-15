using SB.Core;
using System.Collections.Concurrent;
using System.Diagnostics;

namespace SB
{
    public class CompileCommandsEmitter : TaskEmitter
    {
        public CompileCommandsEmitter(IToolchain Toolchain) => this.Toolchain = Toolchain;
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

            var JSON = CLDriver.CompileCommands(Toolchain.Compiler, Target.Directory);
            CompileCommands.Add(JSON);

            sw.Stop();
            Time += (int)sw.ElapsedMilliseconds;
            return null;
        }

        public static void WriteToFile(string Path)
        {
            File.WriteAllText(Path, "[" + String.Join(",", CompileCommands) + "]");
        }

        private static string GetObjectFilePath(Target Target, string SourceFile) => Path.Combine(Target.GetStorePath(BuildSystem.ObjsStore), BuildSystem.GetUniqueTempFileName(SourceFile, Target.Name, "obj"));

        private static ConcurrentBag<string> CompileCommands = new();
        private IToolchain Toolchain { get; }
        public static volatile int Time = 0;
    }
}
