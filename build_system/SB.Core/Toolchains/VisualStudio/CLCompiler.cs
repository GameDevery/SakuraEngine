using Serilog;
using System.Diagnostics;
using System.Text.RegularExpressions;

namespace SB.Core
{
    using VS = VisualStudio;
    public struct CLDependenciesData
    {
        public string Source { get; set; }
        public string ProvidedModule { get; set; }
        public string[] Includes { get; set; }
        public string[] ImportedModules { get; set; }
        public string[] ImportedHeaderUnits { get; set; }
    }

    public struct CLDependencies
    {
        public Version Version { get; set; }
        public CLDependenciesData Data { get; set; }
    }

    public class CLCompiler : ICompiler
    {
        public CLCompiler(string ExePath, Dictionary<string, string?> Env)
        {
            VCEnvVariables = Env;
            this.ExecutablePath = ExePath;

            if (!File.Exists(ExePath))
                throw new ArgumentException($"CLCompiler: ExePath: {ExePath} is not an existed absolute path!");

            this.CLVersionTask = Task.Run(() =>
            {
                Process compiler = new Process
                {
                    StartInfo = new ProcessStartInfo
                    {
                        FileName = ExePath,
                        RedirectStandardError = true,
                        CreateNoWindow = true,
                        UseShellExecute = false
                    }
                };
                compiler.Start();
                var Output = compiler.StandardError.ReadToEnd();
                compiler.WaitForExit();
                // FUCK YOU MICROSOFT THIS IS WEIRD
                Regex pattern = new Regex(@"\d+(\.\d+)+");
                var CLVersion = Version.Parse(pattern.Match(Output).Value);
                Log.Information("CL.exe version ... {CLVersion}", CLVersion);
                return CLVersion;
            });
        }

        public IArgumentDriver CreateArgumentDriver()
        {
            return new CLArgumentDriver();
        }

        public CompileResult Compile(TaskEmitter Emitter, Target Target, IArgumentDriver Driver)
        {
            var CompilerArgsDict = Driver.CalculateArguments();
            var CompilerArgsList = CompilerArgsDict.Values.SelectMany(x => x).ToList();
            var DependArgsList = CompilerArgsList.ToList();
            DependArgsList.Add($"COMPILER:ID={ExecutablePath}");
            DependArgsList.Add($"COMPILER:VERSION={Version}");
            DependArgsList.Add($"ENV:VCToolsVersion={VCEnvVariables["VCToolsVersion"]}");
            DependArgsList.Add($"ENV:WindowsSDKVersion={VCEnvVariables["WindowsSDKVersion"]}");
            DependArgsList.Add($"ENV:WindowsSDKLibVersion={VCEnvVariables["WindowsSDKLibVersion"]}");
            DependArgsList.Add($"ENV:UCRTVersion={VCEnvVariables["UCRTVersion"]}");

            var SourceFile = Driver.Arguments["Source"] as string;
            var ObjectFile = Driver.Arguments["Object"] as string;
            var Changed = Depend.OnChanged(Target.Name, SourceFile!, Emitter.Name, (Depend depend) =>
            {
                Process compiler = new Process
                {
                    StartInfo = new ProcessStartInfo
                    {
                        FileName = ExecutablePath,
                        RedirectStandardInput = false,
                        RedirectStandardOutput = true,
                        RedirectStandardError = true,
                        CreateNoWindow = false,
                        UseShellExecute = false,
                        Arguments = String.Join(" ", CompilerArgsList)
                    }
                };
                foreach (var kvp in VCEnvVariables)
                {
                    compiler.StartInfo.Environment.Add(kvp.Key, kvp.Value);
                }
                compiler.Start();
                var ErrorInfo = compiler.StandardError.ReadToEnd();
                var OutputInfo = compiler.StandardOutput.ReadToEnd();
                compiler.WaitForExit();

                // FUCK YOU MICROSOFT THIS IS WEIRD
                if (compiler.ExitCode != 0)
                {
                    throw new TaskFatalError($"Compile {SourceFile} failed with fatal error!", $"CL.exe: {OutputInfo}");
                }
                else
                {
                    var clDepFilePath = Driver.Arguments["SourceDependencies"] as string;
                    var clDeps = Json.Deserialize<CLDependencies>(File.ReadAllText(clDepFilePath!));
                    depend.ExternalFiles.AddRange(clDeps.Data.Includes);
                }

                if (OutputInfo.Contains("warning"))
                    Log.Warning("CL.exe: {OutputInfo}", OutputInfo);

                depend.ExternalFiles.Add(ObjectFile!);
            }, new List<string> { SourceFile! }, DependArgsList);

            return new CompileResult
            {
                ObjectFile = ObjectFile!,
                PDBFile = Driver.Arguments.TryGetValue("PDB", out var args) ? (string)args! : "",
                IsRestored = !Changed
            };
        }

        public Version Version
        {
            get
            {
                if (!CLVersionTask.IsCompleted)
                    CLVersionTask.Wait();
                return CLVersionTask.Result;
            }
        }

        public readonly Dictionary<string, string?> VCEnvVariables;
        private readonly Task<Version> CLVersionTask;
        public string ExecutablePath { get; }
    }
}
