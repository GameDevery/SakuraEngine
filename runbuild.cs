using SB;
using SB.Core;
using Serilog;
using Serilog.Events;
using System.Diagnostics;

// Main entry point
var parser = new CommandParser();
var mainCmd = new MainCommand();
// Configure the parser
parser.MainCmd(mainCmd, "SB", "Sakura Build System", "SB [options] <command> [command-options]");
return parser.ParseSync(args);

// Main command that holds global options
public class MainCommand
{
    [CmdSub(Name = "build", ShortName = 'b', Help = "Build the project")]
    public BuildCommand Build { get; set; } = new BuildCommand();

    [CmdSub(Name = "test", ShortName = 't', Help = "Run tests")]
    public TestCommand Test { get; set; } = new TestCommand();

    [CmdSub(Name = "clean", ShortName = 'c', Help = "Clean build cache and dependency databases")]
    public CleanCommand Clean { get; set; } = new CleanCommand();

    [CmdSub(Name = "compile_commands", Help = "Generate Compile Commands for IDEs")]
    public CompileCommandsCommand CompileCommands { get; set; } = new CompileCommandsCommand();

    [CmdSub(Name = "vs", Help = "Generate Visual Studio solution")]
    public VSCommand VS { get; set; } = new VSCommand();

    [CmdSub(Name = "vscode", Help = "Generate VSCode debug configurations")]
    public VSCodeCommand VSCode { get; set; } = new VSCodeCommand();

    [CmdSub(Name = "graph", Help = "Generate dependency graph visualization")]
    public GraphCommand Graph { get; set; } = new GraphCommand();
}

public abstract class CommandBase
{
    [CmdOption(Name = "verbose", ShortName = 'v', Help = "Enable verbose logging", IsRequired = false)]
    public bool Verbose { get; set; } = false;

    [CmdOption(Name = "mode", ShortName = 'm', Help = "Build mode (debug/release)", IsRequired = false)]
    public string Mode { get; set; } = "debug";

    [CmdOption(Name = "sha-depend", Help = "Use SHA instead of DateTime for dependency checking", IsRequired = false)]
    public bool UseShaDepend { get; set; } = false;

    [CmdOption(Name = "category", ShortName = 'c', Help = "Build tools", IsRequired = false)]
    public string CategoryString { get; set; } = "modules";

    [CmdOption(Name = "toolchain", Help = "Toolchain to use", IsRequired = false)]
    public string ToolchainName { get; set; } = OperatingSystem.IsWindows() ? "clang-cl" : "clang";

    [CmdOption(Name = "proxy", Help = "Set HTTP proxy for downloads")]
    public string Proxy { get; set; } = "";

    [CmdExec]
    public void Exec()
    {
        Stopwatch timer = Stopwatch.StartNew();

        // Use global verbose setting if available
        LogEventLevel LogLevel = LogEventLevel.Information;
        if (Verbose)
        {
            LogLevel = LogEventLevel.Verbose;
        }
        Engine.InitializeLogger(LogLevel);
        Engine.SetEngineDirectory(SourceLocation.Directory());

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
        _toolchain = Engine.Bootstrap(SourceLocation.Directory(), Categories);

        // run subcmd exec
        OnExecute();

        // stop and dump counters
        timer.Stop();
        Log.Information($"Total: {timer.ElapsedMilliseconds / 1000.0f}s");
        Log.Information($"Execution Total: {timer.ElapsedMilliseconds / 1000.0f}s");
        Log.Information($"Compile Commands Total: {CompileCommandsEmitter.Time / 1000.0f}s");
        Log.Information($"Compile Total: {CppCompileEmitter.Time / 1000.0f}s");
        Log.Information($"Link Total: {CppLinkEmitter.Time / 1000.0f}s");
        Log.CloseAndFlush();
    }

    public abstract void OnExecute();
    public IToolchain Toolchain => _toolchain!;
    private IToolchain? _toolchain;
    protected TargetCategory Categories = TargetCategory.Package;
}

// Build subcommand
public class BuildCommand : CommandBase
{
    [CmdOption(Name = "shader-only", Help = "Build shaders only", IsRequired = false)]
    public bool ShaderOnly { get; set; }

    [CmdOption(Name = "target", Help = "Build a single target", IsRequired = false)]
    public string? SingleTarget { get; set; }

    public override void OnExecute()
    {
        Engine.AddShaderTaskEmitters(Toolchain);
        if (!ShaderOnly)
        {
            Engine.AddEngineTaskEmitters(Toolchain);
        }
        Engine.RunBuild(SingleTarget);

        if (Categories.HasFlag(TargetCategory.Tool))
        {
            string ToolsDirectory = Path.Combine(SourceLocation.Directory(), ".sb", "tools");
            BuildSystem.Artifacts.AsParallel().ForAll(artifact =>
            {
                if (artifact is LinkResult Program)
                {
                    if (!Program.IsRestored && Program.Target.IsCategory(TargetCategory.Tool))
                    {
                        // copy to /.sb/tools
                        if (File.Exists(Program.PDBFile))
                        {
                            Log.Verbose("Copying PDB file {PDBFile} to {ToolsDirectory}", Program.PDBFile, ToolsDirectory);
                            File.Copy(Program.PDBFile, Path.Combine(ToolsDirectory, Path.GetFileName(Program.PDBFile)), true);
                        }
                        if (File.Exists(Program.TargetFile))
                        {
                            Log.Verbose("Copying target file {TargetFile} to {ToolsDirectory}", Program.TargetFile, ToolsDirectory);
                            File.Copy(Program.TargetFile, Path.Combine(ToolsDirectory, Path.GetFileName(Program.TargetFile)), true);
                        }
                    }
                }
            });
        }
    }
}

// Test subcommand
public class TestCommand : CommandBase
{
    public override void OnExecute()
    {
        // Add necessary emitters for test execution
        Engine.AddEngineTaskEmitters(Toolchain);
        Engine.RunBuild();

        // This assumes build has already been done
        var Programs = BuildSystem.Artifacts.Where(a => a is LinkResult)
            .Select(a => (LinkResult)a)
            .Where(p => p.Target.IsCategory(TargetCategory.Tests) && p.Target.GetTargetType() == TargetType.Executable)
            .ToList();

        if (!Programs.Any())
        {
            Log.Warning("No test programs found. Make sure to build tests first.");
            return;
        }

        Programs.AsParallel().ForAll(program =>
        {
            Stopwatch sw = Stopwatch.StartNew();
            Log.Information("Running test target {TargetName}", program.Target.Name);
            ProcessOptions Options = new ProcessOptions
            {
                WorkingDirectory = Path.GetDirectoryName(program.TargetFile)
            };
            var result = BuildSystem.RunProcess(program.TargetFile, "", out var output, out var error, Options);
            sw.Stop();
            float Seconds = sw.ElapsedMilliseconds / 1000.0f;
            if (result != 0)
            {
                Log.Error("Test target {TargetName} failed with error: {Error}", program.Target.Name, error);
            }
            else
            {
                Log.Information("Test target {TargetName} passed, cost {Seconds}s", program.Target.Name, Seconds);
            }
        });
    }
}

// Clean subcommand
public class CleanCommand : CommandBase
{
    [CmdOption(Name = "database", ShortName = 'd', Help = "Database to clean, 'all | packages' | 'targets' | 'shaders' | 'sdks'", IsRequired = false)]
    public string Database { get; set; } = "targets";

    public override void OnExecute()
    {
        Log.Information("Cleaning build cache dependency databases for {Database}...", Database);

        // Clean dependency databases using API
        try
        {
            bool all = (Database == "all");
            if (all || Database == "targets")
                BuildSystem.CppCompileDepends(false).ClearDatabase();
            if (all || Database == "packages")
                BuildSystem.CppCompileDepends(true).ClearDatabase();
            if (all || Database == "shaders")
                Engine.ShaderCompileDepend.ClearDatabase();
            if (all || Database == "misc")
                Engine.MiscDepend.ClearDatabase();
            if (all || Database == "sdks")
            {
                Install.DownloadDepend.ClearDatabase();
                Install.SDKDepend.ClearDatabase();
            }
        }
        catch (Exception ex)
        {
            Log.Error("Failed to clear databases: {Error}", ex.Message);
        }
    }
}

public class CompileCommandsCommand : CommandBase
{
    public CompileCommandsCommand()
    {
        CategoryString = "all";
    }

    public override void OnExecute()
    {
        Engine.AddCompileCommandsEmitter(Toolchain);
        Engine.RunBuild();

        Directory.CreateDirectory(".sb/compile_commands/cpp");
        CompileCommandsEmitter.WriteToFile(".sb/compile_commands/cpp/compile_commands.json");

        Directory.CreateDirectory(".sb/compile_commands/shaders");
        CppSLCompileCommandsEmitter.WriteCompileCommandsToFile(".sb/compile_commands/shaders/compile_commands.json");
    }
}

// VS subcommand
public class VSCommand : CommandBase
{
    [CmdOption(Name = "solution-name", Help = "Name of the solution file (without extension)", IsRequired = false)]
    public string SolutionName { get; set; } = "SakuraEngine";

    [CmdOption(Name = "output", Help = "Output directory for solution files", IsRequired = false)]
    public string OutputDirectory { get; set; } = ".sb/VisualStudioSolution";

    public override void OnExecute()
    {
        Log.Information("Generating Visual Studio solution...");

        // Set output directory for VS emitter
        VSEmitter.RootDirectory = !string.IsNullOrEmpty(Engine.EngineDirectory) ? Engine.EngineDirectory : Directory.GetCurrentDirectory();
        VSEmitter.OutputDirectory = OutputDirectory;

        // Add VS emitter to generate project files
        BuildSystem.AddTaskEmitter("VSEmitter", new VSEmitter(Toolchain));

        // Run the build to process all targets
        Engine.RunBuild();

        // Generate solution file
        var solutionPath = Path.Combine(OutputDirectory, $"{SolutionName}.sln");
        VSEmitter.GenerateSolution(solutionPath, SolutionName);

        Log.Information("Visual Studio solution generated at: {Path}", Path.GetFullPath(solutionPath));
    }
}

// VSCode subcommand
public class VSCodeCommand : CommandBase
{
    [CmdOption(Name = "debugger", Help = "Default debugger type: cppdbg, lldb-dap, codelldb, cppvsdbg", IsRequired = false)]
    public string Debugger { get; set; } = "cppdbg";

    [CmdOption(Name = "workspace", Help = "Workspace root directory", IsRequired = false)]
    public string WorkspaceRoot { get; set; } = ".";

    [CmdOption(Name = "preserve-user", Help = "Preserve user-created debug configurations", IsRequired = false)]
    public bool PreserveUser { get; set; } = true;

    public override void OnExecute()
    {
        Log.Information("Generating VSCode debug configurations...");

        // Set debugger type
        VSCodeDebugEmitter.DebuggerType debuggerType = VSCodeDebugEmitter.DebuggerType.CppDbg;
        switch (Debugger.ToLower())
        {
            case "lldb-dap":
                debuggerType = VSCodeDebugEmitter.DebuggerType.LLDBDap;
                break;
            case "codelldb":
                debuggerType = VSCodeDebugEmitter.DebuggerType.CodeLLDB;
                break;
            case "cppvsdbg":
                debuggerType = VSCodeDebugEmitter.DebuggerType.CppVsDbg;
                break;
        }

        // Configure VSCode emitter
        VSCodeDebugEmitter.WorkspaceRoot = !string.IsNullOrEmpty(WorkspaceRoot) ? WorkspaceRoot : 
                                           (!string.IsNullOrEmpty(Engine.EngineDirectory) ? Engine.EngineDirectory : Directory.GetCurrentDirectory());
        VSCodeDebugEmitter.DefaultDebugger = debuggerType;
        VSCodeDebugEmitter.PreserveUserConfigurations = PreserveUser;

        // Add VSCode emitter
        var emitter = new VSCodeDebugEmitter(Toolchain);
        BuildSystem.AddTaskEmitter("VSCodeDebugEmitter", emitter);

        // Run the build to process all targets
        Engine.RunBuild();

        // Generate the debug configurations
        VSCodeDebugEmitter.GenerateDebugConfigurations();
        
        Log.Information("VSCode debug configurations generated in: {Path}", Path.GetFullPath(Path.Combine(VSCodeDebugEmitter.WorkspaceRoot, ".vscode")));
    }
}

// Graph subcommand
public class GraphCommand : CommandBase
{
    [CmdOption(Name = "format", ShortName = 'f', Help = "Output format: dot, mermaid, plantuml", IsRequired = false)]
    public string Format { get; set; } = "dot";

    [CmdOption(Name = "color", ShortName = 's', Help = "Color scheme: type, module, size, deps", IsRequired = false)]
    public string ColorScheme { get; set; } = "type";

    [CmdOption(Name = "output", ShortName = 'o', Help = "Output directory for graph files", IsRequired = false)]
    public string OutputDirectory { get; set; } = ".sb/graphs";

    [CmdOption(Name = "external", ShortName = 'e', Help = "Include external dependencies", IsRequired = false)]
    public bool IncludeExternalDeps { get; set; } = false;

    [CmdOption(Name = "transitive", ShortName = 't', Help = "Show transitive dependencies (default: false, only direct deps)", IsRequired = false)]
    public bool ShowTransitiveDeps { get; set; } = false;

    [CmdOption(Name = "depth", ShortName = 'd', Help = "Maximum dependency depth (-1 for unlimited)", IsRequired = false)]
    public int MaxDepth { get; set; } = -1;

    [CmdOption(Name = "open", Help = "Open generated graph file after creation", IsRequired = false)]
    public bool OpenAfterGeneration { get; set; } = false;

    public override void OnExecute()
    {
        Log.Information("Generating dependency graph...");

        // Clear any previous data
        DependencyGraphEmitter.Clear();
        
        // Configure the emitter
        DependencyGraphEmitter.Format = ParseFormat(Format);
        DependencyGraphEmitter.ColorScheme = ParseColorScheme(ColorScheme);
        DependencyGraphEmitter.OutputDirectory = OutputDirectory;
        DependencyGraphEmitter.IncludeExternalDeps = IncludeExternalDeps;
        DependencyGraphEmitter.ShowTransitiveDeps = ShowTransitiveDeps;
        DependencyGraphEmitter.MaxDepth = MaxDepth;

        // Add the emitter to the build system
        var emitter = new DependencyGraphEmitter();
        BuildSystem.AddTaskEmitter("DependencyGraph", emitter);

        // Run the build to collect dependency information
        Engine.RunBuild();

        // Generate the dependency graph after build
        DependencyGraphEmitter.GenerateDependencyGraph();

        Log.Information("Dependency graph generated in: {Path}", Path.GetFullPath(OutputDirectory));

        // List generated files
        var graphFiles = Directory.GetFiles(OutputDirectory, "dependency_graph_*.*", SearchOption.TopDirectoryOnly)
            .OrderByDescending(f => File.GetCreationTime(f))
            .Take(2)  // Get the latest graph and stats file
            .ToList();

        foreach (var file in graphFiles)
        {
            Log.Information("  - {File}", Path.GetFileName(file));
        }

        // Optionally open the generated file
        if (OpenAfterGeneration && graphFiles.Any())
        {
            var latestGraph = graphFiles.FirstOrDefault(f => !f.EndsWith("_stats.txt"));
            if (latestGraph != null)
            {
                try
                {
                    if (OperatingSystem.IsWindows())
                    {
                        Process.Start(new ProcessStartInfo
                        {
                            FileName = latestGraph,
                            UseShellExecute = true
                        });
                    }
                    else if (OperatingSystem.IsMacOS())
                    {
                        Process.Start("open", latestGraph);
                    }
                    else if (OperatingSystem.IsLinux())
                    {
                        Process.Start("xdg-open", latestGraph);
                    }
                }
                catch (Exception ex)
                {
                    Log.Warning("Failed to open graph file: {Error}", ex.Message);
                }
            }
        }

        // Provide rendering hints based on format
        if (Format.ToLower() == "dot")
        {
            Log.Information("To render the graph, you can use:");
            Log.Information("  dot -Tpng {OutputDirectory}/dependency_graph_*.dot -o graph.png", OutputDirectory);
            Log.Information("  dot -Tsvg {OutputDirectory}/dependency_graph_*.dot -o graph.svg", OutputDirectory);
        }
        else if (Format.ToLower() == "mermaid")
        {
            Log.Information("To view the Mermaid graph:");
            Log.Information("  - Copy the content to https://mermaid.live/");
            Log.Information("  - Or include in a Markdown file with Mermaid support");
        }
        else if (Format.ToLower() == "plantuml")
        {
            Log.Information("To render the PlantUML graph:");
            Log.Information("  java -jar plantuml.jar {OutputDirectory}/dependency_graph_*.puml", OutputDirectory);
        }
    }

    private DependencyGraphEmitter.GraphFormat ParseFormat(string format)
    {
        return format.ToLower() switch
        {
            "dot" => DependencyGraphEmitter.GraphFormat.DOT,
            "mermaid" => DependencyGraphEmitter.GraphFormat.Mermaid,
            "plantuml" => DependencyGraphEmitter.GraphFormat.PlantUML,
            _ => DependencyGraphEmitter.GraphFormat.DOT
        };
    }

    private DependencyGraphEmitter.NodeColorScheme ParseColorScheme(string scheme)
    {
        return scheme.ToLower() switch
        {
            "type" => DependencyGraphEmitter.NodeColorScheme.ByType,
            "module" => DependencyGraphEmitter.NodeColorScheme.ByModule,
            "size" => DependencyGraphEmitter.NodeColorScheme.BySize,
            "deps" => DependencyGraphEmitter.NodeColorScheme.ByDependencyCount,
            _ => DependencyGraphEmitter.NodeColorScheme.ByType
        };
    }
}