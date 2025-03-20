﻿using Serilog;
using System.Diagnostics;

namespace SB.Core
{
    using VS = VisualStudio;
    public class LINK : ILinker
    {
        public LINK(string ExePath, Dictionary<string, string?> Env)
        {
            VCEnvVariables = Env;
            MSVCVersion = Version.Parse(VCEnvVariables["VCToolsVersion"]!);
            this.ExePath = ExePath;

            if (!File.Exists(ExePath))
                throw new ArgumentException($"LINK: ExePath: {ExePath} is not an existed absolute path!");

            Log.Information("LINK.exe version ... {MSVCVersion}", MSVCVersion);
        }

        public LinkResult Link(TaskEmitter Emitter, Target Target, IArgumentDriver Driver)
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
            bool Changed = Depend.OnChanged(Target.Name, OutputFile!, Emitter.Name, (Depend depend) =>
            {
                var StringLength = LinkerArgsList.Sum(x => x.Length);
                string Arguments = "";
                if (StringLength > 30000)
                {
                    var Content = String.Join("\n", LinkerArgsList);
                    string ResponseFile = Path.Combine(BuildSystem.TempPath, $"{Guid.CreateVersion7()}.txt");
                    File.WriteAllText(ResponseFile, Content);

                    Arguments = $"{TargetTypeArg} @{ResponseFile}";
                }
                else
                {
                    Arguments = $"{TargetTypeArg} {String.Join(" ", LinkerArgsList)}";
                }
                int ExitCode = BuildSystem.RunProcess(ExePath, Arguments, out var OutputInfo, out var ErrorInfo, VCEnvVariables);

                // FUCK YOU MICROSOFT THIS IS WEIRD, WHY YOU DUMP ERRORS THROUGH STDOUT ?
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

        public Version Version => MSVCVersion;

        public readonly Dictionary<string, string?> VCEnvVariables;
        private readonly Version MSVCVersion;
        private readonly string ExePath;
    }
}
