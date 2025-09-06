using SB;
using SB.Core;
using Serilog;
using Serilog.Events;
using System.Diagnostics;
using Cli = SB.Cli;

namespace SB;

public abstract class CommandBase
{
    [Cli.Option(Name = "verbose", ShortName = 'v', Help = "Enable verbose logging", IsRequired = false)]
    public bool Verbose { get; set; } = false;

    [Cli.Option(Name = "mode", ShortName = 'm', Help = "Build mode", IsRequired = false)]
    public string Mode { get; set; } = "debug";
    [Cli.OptionSelectionProvider("mode")]
    public static IEnumerable<string> ModeSelections()
    {
        Engine.LoadConfigurations();
        return Engine.Configurations.Keys;
    }

    [Cli.Option(Name = "sha-depend", Help = "Use SHA instead of DateTime for dependency checking", IsRequired = false)]
    public bool UseShaDepend { get; set; } = false;

    [Cli.Option(Name = "category", ShortName = 'c', Help = "Build by category", IsRequired = false, Selections = ["all", "modules", "tools"])]
    public string CategoryString { get; set; } = "all";

    [Cli.Option(Name = "toolchain", Help = "Toolchain to use", IsRequired = false, Selections = ["msvc", "clang-cl", "clang"])]
    public string ToolchainName { get; set; } = OperatingSystem.IsWindows() ? "clang-cl" : "clang";

    [Cli.Option(Name = "proxy", Help = "Set HTTP proxy for downloads")]
    public string Proxy { get; set; } = "";

    [Cli.ExecCmd]
    public int Exec()
    {
        Stopwatch timer = Stopwatch.StartNew();

        // Use global verbose setting if available
        LogEventLevel LogLevel = LogEventLevel.Information;
        if (Verbose)
        {
            LogLevel = LogEventLevel.Verbose;
        }
        Engine.InitializeLogger(LogLevel);

        // set proxy
        if (!string.IsNullOrEmpty(Proxy))
        {
            Log.Information("Setting HTTP proxy to {Proxy}", Proxy);
            Download.HttpProxy = Proxy;
        }

        // use sha to check file dependency instead of using last write time 
        if (UseShaDepend)
            Depend.DefaultUseSHAInsteadOfDateTime = true;

        // Set compiler
        if (ToolchainName == "clang-cl")
            VisualStudio.UseClangCl = true;
        else if (ToolchainName == "msvc")
            VisualStudio.UseClangCl = false;
        else if (ToolchainName == "clang")
        {
            ; //XCode.
        }

        // Set configuration based on mode
        BuildSystem.GlobalConfiguration = Mode.ToLower();
        if (Mode.ToLower() != "debug" && Mode.ToLower() != "release")
        {
            Log.Warning($"Unknown build mode '{Mode}', defaulting to debug");
            BuildSystem.GlobalConfiguration = "debug";
        }
        Log.Information("Build start with configuration: {Configuration}", BuildSystem.GlobalConfiguration);

        // Set categories
        if (CategoryString == "modules")
            Categories |= TargetCategory.Runtime | TargetCategory.DevTime;
        if (CategoryString == "tools")
            Categories |= TargetCategory.Tool;
        if (CategoryString == "all")
            Categories |= TargetCategory.Tool | TargetCategory.Runtime | TargetCategory.DevTime;
        Log.Information("Build start with categories: {Categories}", Categories);

        // Bootstrap engine
        _toolchain = Engine.Bootstrap(Categories);

        // run subcmd exec
        var returnCode = OnExecute();

        // stop and dump counters
        timer.Stop();
        Log.Information($"Total: {timer.ElapsedMilliseconds / 1000.0f}s");
        Log.Information($"Execution Total: {timer.ElapsedMilliseconds / 1000.0f}s");
        Log.Information($"Compile Commands Total: {CompileCommandsEmitter.Time / 1000.0f}s");
        Log.Information($"Compile Total: {CppCompileEmitter.Time / 1000.0f}s");
        Log.Information($"Link Total: {CppLinkEmitter.Time / 1000.0f}s");
        Log.CloseAndFlush();

        return returnCode;
    }

    public abstract int OnExecute();
    public IToolchain Toolchain => _toolchain!;
    private IToolchain? _toolchain;
    protected TargetCategory Categories = TargetCategory.Package;
}