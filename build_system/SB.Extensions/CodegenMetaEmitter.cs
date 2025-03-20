using SB.Core;
using Serilog;
using System.Diagnostics;

namespace SB
{
    public class CodegenMetaAttribute
    {
        public required string RootDirectory { get; init; }
    }

    public class CodegenMetaEmitter : TaskEmitter
    {
        public CodegenMetaEmitter(IToolchain Toolchain, string ToolDirectory)
        {
            this.Toolchain = Toolchain;
            var ResultList = Directory.GetFiles(ToolDirectory, "meta*", SearchOption.TopDirectoryOnly);
            if (ResultList.Length > 0)
            {
                SearchTask = Task.FromResult(ResultList[0]);
            }
            else
            {
                throw new Exception("meta.exe not found in " + ToolDirectory);
                // TODO: Download if not found
                /*
                SearchTask = Task.Run(() =>
                {
                    Download...
                });
                */
            }
        }
        public override bool EnableEmitter(Target Target) => Target.GetAttribute<CodegenMetaAttribute>() is not null && Target.AllFiles.Any(F => F.EndsWith(".h") || F.EndsWith(".hpp"));
        public override bool EmitTargetTask(Target Target) => true;
        public override IArtifact? PerTargetTask(Target Target)
        {
            // Ensure output file
            var MetaAttribute = Target.GetAttribute<CodegenMetaAttribute>()!;
            var GeneratedDirectory = Path.Combine(Target.GetStorePath(BuildSystem.GeneratedSourceStore), "meta_database");
            Directory.CreateDirectory(GeneratedDirectory);
            var BatchDirectory = Path.Combine(Target.GetStorePath(BuildSystem.GeneratedSourceStore), "ReflectionBatch");
            Directory.CreateDirectory(BatchDirectory);

            // Batch headers to a source file
            var Headers = Target.AllFiles.Where(F => F.EndsWith(".h") || F.EndsWith(".hpp"));
            var BatchFile = Path.Combine(BatchDirectory, "ReflectionBatch.cpp");
            Depend.OnChanged(Target.Name, "", Name, (Depend depend) => {
                Directory.CreateDirectory(BatchDirectory);
                File.WriteAllLines(BatchFile, Headers.Select(H => $"#include \"{H}\""));
            }, Headers.Append(Target.Location), null);

            // Set meta args
            var MetaArgs = new List<string>{
                BatchFile,
                $"--output={GeneratedDirectory}",
                $"--root={MetaAttribute.RootDirectory}",
                "--"
            };
            // Compiler arguments
            var EXE = ExecutablePath;
            var CompilerArgs = Toolchain.Compiler.CreateArgumentDriver()
                .AddArguments(Target.Arguments)
                .CalculateArguments()
                .Values.SelectMany(x => x).ToList();
            // Set addon flags            
            CompilerArgs.Add("-Wno-abstract-final-class");
            if (BuildSystem.TargetOS == OSPlatform.Windows)
                CompilerArgs.Add("--driver-mode=cl");
            MetaArgs.AddRange(CompilerArgs);
            // Run meta.exe
            bool Changed = Depend.OnChanged(Target.Name, GeneratedDirectory, Name, (Depend depend) =>
            {
                int ExitCode = BuildSystem.RunProcess(EXE, string.Join(" ", MetaArgs), out var OutputInfo, out var ErrorInfo);
                if (ExitCode != 0)
                    throw new TaskFatalError($"meta.exe {BatchFile} failed with fatal error!", $"meta.exe: {OutputInfo}");
                else if (OutputInfo.Contains("warning LNK"))
                    Log.Warning("meta.exe: {OutputInfo}", OutputInfo);

                var AllGeneratedFiles = Directory.GetFiles(GeneratedDirectory, "*.meta", SearchOption.AllDirectories);
                depend.ExternalFiles.AddRange(AllGeneratedFiles);
            }, new string[] { BatchFile }, MetaArgs);
            return new MetaArtifact { IsRestored = !Changed };
        }

        private string ExecutablePath
        {
            get
            {
                SearchTask.Wait();
                return SearchTask.Result;
            }
        }
        private IToolchain Toolchain { get; }
        private Task<string> SearchTask;
        public static volatile int Time = 0;
    }

    public class MetaArtifact : IArtifact
    {
        public bool IsRestored { get; init; }
    }
}