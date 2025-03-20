using Serilog;
using System.Text.RegularExpressions;

namespace SB.Core
{
    using VS = VisualStudio;
    public class LD : ILinker
    {
        public LD(string ExePath, XCode Toolchain)
        {
            this.ExecutablePath = ExePath;
            this.XCodeToolchain = Toolchain!;

            if (!File.Exists(ExePath))
                throw new ArgumentException($"ClangCLCompiler: ExePath: {ExePath} is not an existed absolute path!");

            this.LDVersionTask = Task.Run(() =>
            {
                int ExitCode = BuildSystem.RunProcess(ExePath, "-v", out var Output, out var Error);
                if (ExitCode == 0)
                {
                    Regex pattern = new Regex(@"\d+(\.\d+)+");
                    var LDVersion = Version.Parse(pattern.Match(Error).Value);
                    Log.Information("ld version ... {LDVersion}", LDVersion);
                    return LDVersion;
                }
                else
                {
                    throw new Exception($"Failed to get clang.exe version! Exit code: {ExitCode}, Error: {Error}");
                }
            });
        }

        public LinkResult Link(TaskEmitter Emitter, Target Target, IArgumentDriver Driver)
        {
            var LinkerArgsDict = Driver.CalculateArguments();
            var LinkerArgsList = LinkerArgsDict.Values.SelectMany(x => x).ToList();
            var DependArgsList = LinkerArgsList.ToList();
            DependArgsList.Add($"LINKER:ID=LD");
            DependArgsList.Add($"LINKER:VERSION={Version}");
            DependArgsList.Add($"ENV:SDKVersion={XCodeToolchain.SDKVersion}");

            var InputFiles = Driver.Arguments["Inputs"] as ArgumentList<string>;
            var OutputFile = Driver.Arguments["Output"] as string;
            bool Changed = Depend.OnChanged(Target.Name, OutputFile!, Emitter.Name, (Depend depend) =>
            {
                int ExitCode = BuildSystem.RunProcess(ExecutablePath, String.Join(" ", LinkerArgsList), out var OutputInfo, out var ErrorInfo);
                if (ExitCode != 0)
                    throw new TaskFatalError($"Link {OutputFile} failed with fatal error!", $"LINK.exe: {OutputInfo}");
                else if (OutputInfo.Contains("warning LNK"))
                    Log.Warning("LINK.exe: {OutputInfo}", OutputInfo);

                depend.ExternalFiles.AddRange(OutputFile!);
            }, new List<string>(InputFiles!), DependArgsList);

            return new LinkResult
            {
                TargetFile = LinkerArgsDict["Output"][0],
                PDBFile = Driver.Arguments.TryGetValue("PDB", out var args) ? (string)args! : "",
                IsRestored = !Changed
            };
        }

        public IArgumentDriver CreateArgumentDriver() => new LINKArgumentDriver();
        public Version Version
        {
            get
            {
                LDVersionTask.Wait();
                return LDVersionTask.Result;
            }
        }
        private XCode XCodeToolchain;
        private readonly string ExecutablePath;
        private readonly Task<Version> LDVersionTask;
    }
}
