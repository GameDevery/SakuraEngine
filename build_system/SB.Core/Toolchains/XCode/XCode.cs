using System.Diagnostics;
using Microsoft.Extensions.FileSystemGlobbing;
using Serilog;

namespace SB.Core
{
    public partial class XCode : IToolchain
    {
        public Task<bool> Initialize()
        {
            if (BuildSystem.TargetOS != OSPlatform.OSX)
                throw new Exception("We only support macosx native compilation for now!");
            InitializeTask = Task.Run(() =>
            {
                FastPathFind();
                XCRunFind();
                InitializeTools();
                return true;
            });
            return InitializeTask;
        }

        void FastPathFind()
        {
            HasCommandLineTools = Directory.Exists(CommandLineToolsDirectory);
            HasXCodeIDE = Directory.Exists(XCodeDirectory);
            if (!HasCommandLineTools && !HasXCodeIDE)
                return;
            var RootDirectoryToFind = HasCommandLineTools ? CommandLineToolsDirectory : XCodeDirectory;
            // find tools
            HasClang = File.Exists(ClangDirectory = Path.Combine(RootDirectoryToFind, "usr/bin/clang"));
            HasLD = File.Exists(LDDirectory = Path.Combine(RootDirectoryToFind, "usr/bin/ld"));
            HasAR = File.Exists(ARDirectory = Path.Combine(RootDirectoryToFind, "usr/bin/ar"));
            // find sdks
            var SDKs = Directory.GetDirectories(Path.Combine(RootDirectoryToFind, "SDKs"));
            var LatestVersion = SDKs.Select(P => Path.GetFileName(P).Replace("MacOSX", "").Replace(".sdk", "")).OrderByDescending(P => P).First();
            SDKVersion = Version.Parse(LatestVersion);
            PlatSDKDirectory = Path.Combine(RootDirectoryToFind, "SDKs", $"MacOSX{SDKVersion}.sdk");
        }

        void XCRunFind()
        {
            if (PlatSDKDirectory is null)
            {
                BuildSystem.RunProcess("xcrun", "-sdk macosx --show-sdk-path", out string? output, out string? error);
                PlatSDKDirectory = output.Trim();
            }
            if (SDKVersion is null)
            {
                BuildSystem.RunProcess("xcrun", "-sdk macosx --show-sdk-version", out string? output, out string? error);
                SDKVersion = Version.Parse(output);
            }
            if (!HasCommandLineTools && !HasXCodeIDE)
            {
                if (PlatSDKDirectory.Contains("Xcode.app"))
                {
                    XCodeDirectory = Directory.GetParent(PlatSDKDirectory)!.FullName;
                    HasXCodeIDE = true;
                }
                else
                {
                    CommandLineToolsDirectory = Directory.GetParent(PlatSDKDirectory)!.FullName;
                    HasCommandLineTools = true;
                }
                if (!HasCommandLineTools && !HasXCodeIDE)
                    throw new Exception("Failed to detect XCode or CommandLineTools!");
            }
            if (!HasClang)
            {
                BuildSystem.RunProcess("xcrun", "-sdk macosx --find clang", out string? output, out string? error);
                ClangDirectory = output.Trim();
                HasClang = true;
            }
            if (!HasLD)
            {
                BuildSystem.RunProcess("xcrun", "-sdk macosx --find ld", out string? output, out string? error);
                LDDirectory = output.Trim();
                HasLD = true;
            }
            if (!HasAR)
            {
                BuildSystem.RunProcess("xcrun", "-sdk macosx --find ar", out string? output, out string? error);
                ARDirectory = output.Trim();
                HasAR = true;
            }
        }

        void InitializeTools()
        {
            AppleClang = new AppleClangCompiler(ClangDirectory!, this);
            AppleLD = new LD(LDDirectory!, this);
        }

        public Version Version
        {
            get
            {
                InitializeTask!.Wait();
                return SDKVersion!;
            }
        }
        public ICompiler Compiler
        {
            get
            {
                InitializeTask!.Wait();
                return AppleClang!;
            }
        }
        public ILinker Linker
        {
            get
            {
                InitializeTask!.Wait();
                return AppleLD!;
            }
        }
        public bool HasCommandLineTools { get; private set; } = false;
        public string CommandLineToolsDirectory { get; private set; } = "/Library/Developer/CommandLineTools/";
        public bool HasXCodeIDE { get; private set; } = false;
        public string XCodeDirectory { get; private set; } = "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/";
        // Auto detect:
        // xcrun -sdk macosx --show-sdk-path
        // From XCode:
        // /Applications/Xcode*.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX*.*.sdk
        // /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX*.*.sdk
        // From CommandLineTools:
        // /Library/Developer/CommandLineTools/SDKs/MacOSX15.2.sdk/System/Library/Frameworks/CoreFoundation.framework/Versions/A/Headers
        internal string? PlatSDKDirectory { get; set; }
        public bool HasClang { get; private set; } = false;
        public string? ClangDirectory { get; private set; }
        public AppleClangCompiler? AppleClang { get; private set; }
        public LD? AppleLD { get; private set; }
        public bool HasAR { get; private set; } = false;
        public string? ARDirectory { get; private set; }
        public bool HasLD { get; private set; } = false;
        public string? LDDirectory { get; private set; }
        private Task<bool>? InitializeTask { get; set; }
        internal Version? SDKVersion = null;
    }
}