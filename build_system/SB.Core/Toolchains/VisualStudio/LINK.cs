using Serilog;
using System.Diagnostics;

namespace SB.Core
{
    using VS = VisualStudio;
    public class LINK : ILinker
    {
        public LINK(string ExePath, Dictionary<string, string?> Env)
        {
            VCEnvVariables = Env;
            MSVCVersion = Version.Parse(VCEnvVariables["VCToolsVersion"]);
            this.ExePath = ExePath;

            if (!File.Exists(ExePath))
                throw new ArgumentException($"LINK: ExePath: {ExePath} is not an existed absolute path!");

            Log.Information("LINK.exe version ... {MSVCVersion}", MSVCVersion);
        }

        public LinkResult Link(string TargetName, string EmitterName, IArgumentDriver Driver)
        {
            var LinkerArgsDict = Driver.CalculateArguments();

            // FUCK YOU MICROSOFT AGAIN AND AGAIN
            // WE CAN NOT PUT /LIB, /DLL INTO ARGS.txt
            var TargetTypeArg = LinkerArgsDict["TargetType"][0];
            LinkerArgsDict.Remove("TargetType");

            var LinkerArgsList = LinkerArgsDict.Values.SelectMany(x => x).ToList();
            var DependArgsList = LinkerArgsList.ToList();
            // Version of LINE.exe may change
            DependArgsList.Add($"ENV:VCToolsVersion={VCEnvVariables["VCToolsVersion"]}");
            // LINE.exe links against Windows DLLs with syslinks so we need to add the version in deps
            DependArgsList.Add($"ENV:WindowsSDKVersion={VCEnvVariables["WindowsSDKVersion"]}");
            // LINE.exe links against system CRT libs implicitly so we need to add the version in deps
            DependArgsList.Add($"ENV:UCRTVersion={VCEnvVariables["UCRTVersion"]}"); 

            var InputFiles = Driver.Arguments["Inputs"] as ArgumentList<string>;
            var OutputFile = Driver.Arguments["Output"] as string;
            bool Changed = Depend.OnChanged(TargetName, OutputFile, EmitterName, (Depend depend) =>
            {
                var Arguments = String.Join("\n", LinkerArgsList);
                string ResponseFile = Path.Combine(BuildSystem.TempPath, $"{Guid.CreateVersion7()}.txt");
                File.WriteAllText(ResponseFile, Arguments);
                Process linker = new Process
                {
                    StartInfo = new ProcessStartInfo
                    {
                        FileName = ExePath,
                        RedirectStandardInput = false,
                        RedirectStandardOutput = true,
                        RedirectStandardError = true,
                        CreateNoWindow = false,
                        UseShellExecute = false,
                        Arguments = $"{TargetTypeArg} @{ResponseFile}"
                    }
                };
                foreach (var kvp in VCEnvVariables)
                {
                    linker.StartInfo.Environment.Add(kvp.Key, kvp.Value);
                }
                linker.Start();
                var OutputInfo = linker.StandardOutput.ReadToEnd();
                var ErrorInfo = linker.StandardError.ReadToEnd();
                linker.WaitForExit();

                // FUCK YOU MICROSOFT THIS IS WEIRD
                if (linker.ExitCode != 0)
                    throw new TaskFatalError($"Link {OutputFile} failed with fatal error!", $"LINK.exe: {OutputInfo}");
                else if (OutputInfo.Contains("warning LNK"))
                    Log.Warning("LINK.exe: {OutputInfo}", OutputInfo);

                depend.ExternalFiles.AddRange(OutputFile);
            }, new List<string>(InputFiles), DependArgsList);

            return new LinkResult
            {
                TargetFile = LinkerArgsDict["Output"][0],
                PDBFile = Driver.Arguments.TryGetValue("PDB", out var args) ? args as string : "",
                IsRestored = !Changed
            };
        }

        public Version Version => MSVCVersion;

        public readonly Dictionary<string, string?> VCEnvVariables;
        private readonly Version MSVCVersion;
        private readonly string ExePath;
    }
}
