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
        public override bool EnableEmitter(Target Target) => Target.HasFilesOf<CppFileList>() || Target.HasFilesOf<CFileList>();
        public override bool EmitFileTask(Target Target, FileList FileList) => FileList.Is<CppFileList>() || FileList.Is<CFileList>();
        public override IArtifact? PerFileTask(Target Target, string SourceFile)
        {
            Stopwatch sw = new();
            sw.Start();

            var M = Interlocked.Add(ref N, 1);
            Log.Verbose("Compiling {SourceFile} ({N})", SourceFile, M + 1);

            var SourceDependencies = Path.Combine(Target.GetStorePath(BuildSystem.DepsStore), BuildSystem.GetUniqueTempFileName(SourceFile, Target.Name + this.Name, "source.deps.json"));
            var ObjectFile = GetObjectFilePath(Target, SourceFile);
            var CompilerDriver = Toolchain.Compiler.CreateArgumentDriver()
                .AddArguments(Target.Arguments)
                .AddArgument("Source", SourceFile)
                .AddArgument("Object", ObjectFile)
                .AddArgument("SourceDependencies", SourceDependencies);
            if (SourceFile.EndsWith(".cpp") || SourceFile.EndsWith(".cc"))
            {
                var UsePCH = Target.GetAttribute<UsePCHAttribute>();
                if (UsePCH?.CppPCHAST is not null)
                {
                    CompilerDriver.AddArgument("UsePCHAST", UsePCH.CppPCHAST);
                }
            }
            if (WithDebugInfo)
            {
                if (BuildSystem.TargetOS == OSPlatform.Windows)
                    CompilerDriver.AddArgument("PDBMode", PDBMode.Embed);// /Z7
                else
                    Log.Warning("Debug info is not supported on this platform!");
            }
            var R = Toolchain.Compiler.Compile(this, Target, CompilerDriver);
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

    public class CFileList : FileList {}
    public class CppFileList : FileList {}

    public static partial class TargetExtensions
    {
        public static Target AddCppFiles(this Target @this, params string[] Files)
        {
            @this.FileList<CppFileList>().AddFiles(Files);
            return @this;
        }

        public static Target AddCFiles(this Target @this, params string[] Files)
        {
            @this.FileList<CFileList>().AddFiles(Files);
            return @this;
        }
    }
}