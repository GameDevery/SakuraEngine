using System.Collections.Concurrent;
using SB.Core;
using Serilog;

namespace SB
{
    public class CppSLEmitter : TaskEmitter
    {
        public override bool EnableEmitter(Target Target) => Target.HasFilesOf<CppSLFileList>();
        public override bool EmitFileTask(Target Target, FileList FileList) => FileList.Is<CppSLFileList>();
        public override IArtifact? PerFileTask(Target Target, FileList FileList, FileOptions? Options, string SourceFile)
        {
            var CppSLFileList = FileList as CppSLFileList;
            string SourceName = Path.GetFileNameWithoutExtension(SourceFile);
            var OutputDirectory = Path.Combine(Engine.BuildPath, ShaderOutputDirectories[Target.Name]);
            Directory.CreateDirectory(OutputDirectory);

            string Executable = CppSLCompiler;
            if (BuildSystem.TargetOS == OSPlatform.Windows)
                Executable += ".exe";

            bool Changed = Engine.ShaderCompileDepend.OnChanged(Target.Name, SourceFile, "CPPSL", (Depend depend) =>
            {
                var Arguments = new string[]
                {
                    $"--extra-arg=-I{Engine.EngineDirectory}/tools/shader_compiler/LLVM/test_shaders",
                    SourceFile
                };

                ProcessOptions Options = new ProcessOptions
                {
                    WorkingDirectory = OutputDirectory
                };
                int ExitCode = BuildSystem.RunProcess(Executable, string.Join(" ", Arguments), out var Output, out var Error, Options);
                if (ExitCode != 0)
                {
                    throw new TaskFatalError($"Compile CppSL for {SourceFile} failed with fatal error!", $"CppSLCompiler.exe: {Error}");
                }
                else
                {
                    var DepFilePath = Path.Combine(OutputDirectory, $"{SourceName}.d");
                    // line0: {target}.o: {target}.cpp \
                    // line1~n: {include_file} \
                    var AllLines = File.ReadAllLines(DepFilePath!).Select(
                        x => x.Replace("\\ ", " ").Replace(" \\", "").Trim()
                    ).ToArray();
                    var DepIncludes = new Span<string>(AllLines, 1, AllLines.Length - 1);
                    depend.ExternalFiles.AddRange(DepIncludes);
                }

                // Get all files under output directory that matches '{SourceName}.*.*.hlsl' or '{SourceName}.*.*.metal'
                var OutputFiles = Directory.GetFiles(OutputDirectory, $"{SourceName}.*.*.hlsl")
                    .Concat(Directory.GetFiles(OutputDirectory, $"{SourceName}.*.*.metal"))
                    .ToArray();
                depend.ExternalFiles.AddRange(OutputFiles);
            }, new string[] { Executable, SourceFile }, null);

            if (BuildSystem.TargetOS == OSPlatform.Windows)
            {
                var OutputFiles = Directory.GetFiles(OutputDirectory, $"{SourceName}.*.*.hlsl");
                foreach (var HLSL in OutputFiles)
                {
                    Changed |= !DXCEmitter.CompileHLSL(Target, HLSL, "", OutputDirectory)!.IsRestored;
                }
            }
            else if (BuildSystem.TargetOS == OSPlatform.OSX)
            {
                var OutputFiles = Directory.GetFiles(OutputDirectory, $"{SourceName}.*.*.metal");
                foreach (var Metal in OutputFiles)
                {
                    Changed |= !MSLEmitter.CompileMetal(Target, Metal, "main", OutputDirectory)!.IsRestored;
                }
            }

            return new PlainArtifact { IsRestored = !Changed };
        }
        public static string CppSLCompiler = Path.Combine(Engine.TempPath, "tools", "CppSLCompiler");
        public static Dictionary<string, string> ShaderOutputDirectories = new();
    }

    public class CppSLCompileCommandsEmitter : TaskEmitter
    {
        public override bool EnableEmitter(Target Target) => Target.HasFilesOf<CppSLFileList>();
        public override bool EmitFileTask(Target Target, FileList FileList) => FileList.Is<CppSLFileList>();
        public override IArtifact? PerFileTask(Target Target, FileList FileList, FileOptions? Options, string SourceFile)
        {
            var OutputDirectory = Path.Combine(Engine.BuildPath, CppSLEmitter.ShaderOutputDirectories[Target.Name]);
            Directory.CreateDirectory(OutputDirectory);

            var CMD = new CompileCommand
            {
                directory = OutputDirectory,
                file = SourceFile,
                arguments = new List<string>
                {
                    "clangd",
                    "-std=c++23",
                    "-x", "c++",
                    "-fsyntax-only",
                    "-fms-extensions",
                    "-fms-compatibility-version=17.1.1",
                    "-Wno-microsoft-union-member-reference",
                    $"-I{Engine.EngineDirectory}/tools/shader_compiler/LLVM/test_shaders",
                    SourceFile
                }
            };
            CompileCommands.Add(SB.Core.Json.Serialize(CMD));

            return new PlainArtifact { IsRestored = false };
        }
        public static void WriteCompileCommandsToFile(string Path)
        {
            File.WriteAllText(Path, "[" + String.Join(",", CompileCommands) + "]");
        }
        public static ConcurrentBag<string> CompileCommands = new();
    }

    public class CppSLFileList : FileList
    {

    }

    public static partial class TargetExtensions
    {
        public static Target AddCppSLFiles(this Target @this, params string[] Files)
        {
            @this.FileList<CppSLFileList>().AddFiles(Files);
            if (!CppSLEmitter.ShaderOutputDirectories.ContainsKey(@this.Name))
                CppSLEmitter.ShaderOutputDirectories.Add(@this.Name, "resources/shaders");
            return @this;
        }

        public static Target CppSLOutputDirectory(this Target @this, string Directory)
        {
            CppSLEmitter.ShaderOutputDirectories[@this.Name] = Directory;
            return @this;
        }
    }

}