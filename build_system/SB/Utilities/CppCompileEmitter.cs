using SB.Core;
using System.Collections.Concurrent;
using System.Diagnostics;

namespace SB
{
    public class CppCompileAttribute
    {
        public ConcurrentBag<string> ObjectFiles = new();
    }

    public class CppCompileEmitter : TaskEmitter
    {
        public CppCompileEmitter(IToolchain Toolchain)
        {
            this.Toolchain = Toolchain;
        }
        public override bool EnableEmitter(Target Target) => Target.AllFiles.Any(F => F.Is_C_Cpp() || F.Is_OC_OCpp());
        public override bool EmitTargetTask(Target Target) => true;
        public override IArtifact? PerTargetTask(Target Target)
        {
            Target.SetAttribute(new CppCompileAttribute());
            return null;
        }
        public override bool EmitFileTask(Target Target) => true;
        public override bool FileFilter(Target Target, string File) => Target.GetTargetType() != TargetType.HeaderOnly && File.Is_C_Cpp() || File.Is_OC_OCpp();
        public override IArtifact? PerFileTask(Target Target, string SourceFile)
        {
            Stopwatch sw = new();
            sw.Start();

            var SourceDependencies = Path.Combine(Target.GetStorePath(BuildSystem.DepsStore), BuildSystem.GetUniqueTempFileName(SourceFile, Target.Name + this.Name, "source.deps.json"));
            var ObjectFile = GetObjectFilePath(Target, SourceFile);
            var CLDriver = Toolchain.Compiler.CreateArgumentDriver()
                .AddArguments(Target.Arguments)
                .AddArgument("Source", SourceFile)
                .AddArgument("Object", ObjectFile)
                .AddArgument("SourceDependencies", SourceDependencies);
            var R = Toolchain.Compiler.Compile(this, Target, CLDriver);
            var CompileAttribute = Target.GetAttribute<CppCompileAttribute>()!;
            CompileAttribute.ObjectFiles.Add(ObjectFile);

            sw.Stop();
            Time += (int)sw.ElapsedMilliseconds;
            return R;
        }

        public static string GetObjectFilePath(Target Target, string SourceFile) => Path.Combine(Target.GetStorePath(BuildSystem.ObjsStore), BuildSystem.GetUniqueTempFileName(SourceFile, Target.Name, "obj"));

        private IToolchain Toolchain { get; }
        public static volatile int Time = 0;
    }
}