using Microsoft.Extensions.FileSystemGlobbing;
using System.Text;
using System.Diagnostics;
using Serilog;

namespace SB.Core
{
    public partial class VisualStudio : IToolchain
    {
        // https://blog.pcitron.fr/2022/01/04/dont-use-vcvarsall-vsdevcmd/
        public bool FastFind => VSVersion == 2022;

        public VisualStudio(int VSVersion = 2022, Architecture? HostArch = null, Architecture? TargetArch = null)
        {
            this.VSVersion = VSVersion;
            this.HostArch = HostArch ?? HostInformation.HostArch;
            this.TargetArch = TargetArch ?? HostInformation.HostArch;
        }

        public async Task<bool> Initialize()
        {
            return await Task.Run<bool>(() =>
            {
                FindVCVars();
                RunVCVars();
                return true;
            });
        }

        public Version Version => new Version(VSVersion, 0);
        public ICompiler Compiler => CLCC;
        public ILinker Linker => LINK;
        public string BuildTempPath => Directory.CreateDirectory(Path.Combine(SourceLocation.BuildTempPath, this.Version.ToString())).FullName;

        private void FindVCVars()
        {
            Log.Information("VisualStudio version ... {VSVersion}", VSVersion);

            var matcher = new Matcher();
            if (FastFind)
            {
                matcher.AddIncludePatterns(new[] { "./**/Tools/vsdevcmd/ext/vcvars.bat" });
                matcher.AddIncludePatterns(new[] { "./**/Tools/vsdevcmd/core/winsdk.bat" });
            }
            else
            {
                matcher.AddIncludePatterns(new[] { "./**/VC/Auxiliary/Build/vcvarsall.bat" });
            }
            foreach (var Disk in Windows.EnumLogicalDrives())
            {
                bool FoundVS = false;
                var VersionPostfix = (VSVersion == 2022) ? "/2022" : "";
                var searchDirectory = $"{Disk}:/Program Files/Microsoft Visual Studio{VersionPostfix}";
                IEnumerable<string> matchingFiles = matcher.GetResultsInFullPath(searchDirectory);
                foreach (string file in matchingFiles)
                {
                    var FileName = Path.GetFileName(file);
                    switch (FileName)
                    {
                        case "vcvarsall.bat":
                            FoundVS = true;
                            VCVarsAllBat = file;
                            break;
                        case "vcvars.bat":
                            FoundVS = true;
                            VCVarsBat = file;
                            break;
                        case "winsdk.bat":
                            FoundVS = true;
                            WindowsSDKBat = file;
                            break;
                    }
                }
                if (FoundVS)
                {
                    // "*/Visual Studio/2022/*/**"
                    var SomeBatPath = VCVarsAllBat ?? VCVarsBat;
                    var PayInfo = SomeBatPath
                        .Replace("\\", "/")
                        .Replace(searchDirectory, "")
                        .Split('/')[1];

                    VSInstallDir = $"{searchDirectory}/{PayInfo}/";
                    break;
                }
            }
        }

        static readonly Dictionary<Architecture, string> archStringMap = new Dictionary<Architecture, string> { { Architecture.X86, "x86" }, { Architecture.X64, "x64" }, { Architecture.ARM64, "arm64" } };
        private void RunVCVars()
        {
            var oldEnvPath = Path.Combine(Path.GetTempPath(), $"vcvars_{VSVersion}_prev_{HostArch}_{TargetArch}.txt");
            var newEnvPath = Path.Combine(Path.GetTempPath(), $"vcvars_{VSVersion}_post_{HostArch}_{TargetArch}.txt");

            Process cmd = new Process
            {
                StartInfo = new ProcessStartInfo
                {
                    FileName = "cmd.exe",
                    RedirectStandardInput = false,
                    RedirectStandardOutput = false,
                    RedirectStandardError = false,
                    CreateNoWindow = true,
                    UseShellExecute = false
                }
            };
            if (FastFind)
            {
                cmd.StartInfo.Environment.Add("VSCMD_ARG_HOST_ARCH", archStringMap[HostArch]);
                cmd.StartInfo.Environment.Add("VSCMD_ARG_TGT_ARCH", archStringMap[TargetArch]);
                cmd.StartInfo.Environment.Add("VSCMD_ARG_APP_PLAT", "Desktop");
                cmd.StartInfo.Environment.Add("VSINSTALLDIR", VSInstallDir.Replace("/", "\\"));
                cmd.StartInfo.Arguments = $"/c set > \"{oldEnvPath}\" && \"{VCVarsBat}\" && \"{WindowsSDKBat}\" && set > \"{newEnvPath}\"";
            }
            else
            {
                string ArchString = (TargetArch == HostArch) ? archStringMap[TargetArch] : $"{archStringMap[HostArch]}_{archStringMap[TargetArch]}";
                cmd.StartInfo.Arguments = $"/c set > \"{oldEnvPath}\" && \"{VCVarsAllBat}\" {ArchString} && set > \"{newEnvPath}\"";
            }
            cmd.Start();
            cmd.WaitForExit();

            var oldEnv = EnvReader.Load(oldEnvPath);
            VCEnvVariables = EnvReader.Load(newEnvPath);
            // Preprocess: cull old env variables
            foreach (var oldVar in oldEnv)
            {
                if (VCEnvVariables.ContainsKey(oldVar.Key) && VCEnvVariables[oldVar.Key] == oldEnv[oldVar.Key])
                    VCEnvVariables.Remove(oldVar.Key);
            }
            // Preprocess: cull user env variables
            var vcPaths = VCEnvVariables["Path"].Split(';').ToHashSet();
            vcPaths.ExceptWith(oldEnv["Path"].Split(';').ToHashSet());
            VCEnvVariables["Path"] = string.Join(";", vcPaths);
            // Preprocess: calculate include dir
            var OriginalIncludes = VCEnvVariables.TryGetValue("INCLUDE", out var V0) ? V0 : "";
            var VCVarsIncludes = VCEnvVariables.TryGetValue("__VSCMD_VCVARS_INCLUDE", out var V1) ? V1 : "";
            var WindowsSDKIncludes = VCEnvVariables.TryGetValue("__VSCMD_WINSDK_INCLUDE", out var V2) ? V2 : "";
            var NetFXIncludes = VCEnvVariables.TryGetValue("__VSCMD_NETFX_INCLUDE", out var V3) ? V3 : "";
            VCEnvVariables["INCLUDE"] = VCVarsIncludes + WindowsSDKIncludes + NetFXIncludes + OriginalIncludes;
            // Enum all files and pick usable tools
            foreach (var path in vcPaths)
            {
                foreach (var file in Directory.EnumerateFiles(path))
                {
                    if (Path.GetFileName(file) == "cl.exe")
                        CLCCPath = file;
                    if (Path.GetFileName(file) == "link.exe")
                        LINKPath = file;
                }
            }

            CLCC = new CLCompiler(CLCCPath, VCEnvVariables);
            LINK = new LINK(LINKPath, VCEnvVariables);
        }
        
        public readonly int VSVersion;
        public readonly Architecture HostArch;
        public readonly Architecture TargetArch;

        public string? VSInstallDir { get; private set; }
        public string? VCVarsAllBat { get; private set; }
        public string? VCVarsBat { get; private set; }
        public string? WindowsSDKBat { get; private set; }

        public Dictionary<string, string?> VCEnvVariables { get; private set; }
        public CLCompiler CLCC { get; private set; }
        public LINK LINK { get; private set; }
        public string CLCCPath { get; private set; }
        public string LINKPath { get; private set; }

        #region HelpersForTools
        public static bool CheckPath(string P, bool MustExist) => Path.IsPathFullyQualified(P) && (!MustExist || Directory.Exists(P));
        public static bool CheckFile(string P, bool MustExist) => Path.IsPathFullyQualified(P) && (!MustExist || File.Exists(P));
        public static bool IsValidRT(string what) => ValidRuntimeArguments.Contains(what);
        private static readonly string[] ValidRuntimeArguments = ["MT", "MTd", "MD", "MDd"];
        #endregion
    }
}