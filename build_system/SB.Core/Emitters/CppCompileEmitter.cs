using SB.Core;
using Serilog;
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
        public CppCompileEmitter(IToolchain Toolchain) => this.Toolchain = Toolchain;

        public override bool EnableEmitter(Target Target) => Target.AllFiles.Any(F => F.Is_C_Cpp() || F.Is_OC_OCpp());
        public override bool EmitFileTask(Target Target) => true;
        public override bool FileFilter(Target Target, string File) => Target.GetTargetType() != TargetType.HeaderOnly && File.Is_C_Cpp() || File.Is_OC_OCpp();
        public override IArtifact? PerFileTask(Target Target, string SourceFile)
        {
            Stopwatch sw = new();
            sw.Start();

            var M = Interlocked.Add(ref N, 1);
            Log.Verbose("Compiling {SourceFile} ({N})", SourceFile, M + 1);

            var SourceDependencies = Path.Combine(Target.GetStorePath(BuildSystem.DepsStore), BuildSystem.GetUniqueTempFileName(SourceFile, Target.Name + this.Name, "source.deps.json"));
            var ObjectFile = GetObjectFilePath(Target, SourceFile);
            var CLDriver = Toolchain.Compiler.CreateArgumentDriver()
                .AddArguments(Target.Arguments)
                .AddArgument("Source", SourceFile)
                .AddArgument("Object", ObjectFile)
                .AddArgument("SourceDependencies", SourceDependencies);
            if (WithDebugInfo)
            {
                if (BuildSystem.TargetOS == OSPlatform.Windows)
                    CLDriver.AddArgument("PDBMode", PDBMode.Embed);// /Z7
                else
                    Log.Warning("Debug info is not supported on this platform!");
            }
            var R = Toolchain.Compiler.Compile(this, Target, CLDriver);
            var CompileAttribute = Target.GetAttribute<CppCompileAttribute>()!;
            CompileAttribute.ObjectFiles.Add(ObjectFile);

            Interlocked.Add(ref N, -1);

            sw.Stop();
            Time += (int)sw.ElapsedMilliseconds;
            return R;
        }

        internal int N = 0;
        public static string GetObjectFilePath(Target Target, string SourceFile) => Path.Combine(Target.GetStorePath(BuildSystem.ObjsStore), BuildSystem.GetUniqueTempFileName(SourceFile, Target.Name, "obj"));

        private IToolchain Toolchain { get; }
        public static volatile int Time = 0;
        public static bool WithDebugInfo = false;
    }
}