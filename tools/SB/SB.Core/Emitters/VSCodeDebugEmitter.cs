using SB.Core;
using System;
using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;
using System.Text.Json.Serialization;
using System.Collections.Concurrent;
using System.Linq;
using System.Collections.Generic;
using System.IO;
using Serilog;

namespace SB
{
    using BS = BuildSystem;

    /// <summary>
    /// Emitter for generating VSCode debug launch configurations
    /// Supports multiple debuggers including cppdbg, lldb-dap, CodeLLDB
    /// </summary>
    public class VSCodeDebugEmitter : TaskEmitter
    {
        public enum DebuggerType { CppDbg, LLDBDap, CodeLLDB, CppVsDbg }

        // VSCode configuration file classes
        public class LaunchJson
        {
            [JsonPropertyName("version")]
            public string Version { get; set; } = "0.2.0";
            
            [JsonPropertyName("configurations")]
            public List<DebugConfigurationBase> Configurations { get; set; } = new();
            
            [JsonExtensionData]
            public Dictionary<string, JsonElement>? ExtensionData { get; set; }
        }
        
        public class TasksJson
        {
            [JsonPropertyName("version")]
            public string Version { get; set; } = "2.0.0";
            
            [JsonPropertyName("tasks")]
            public List<TaskConfiguration> Tasks { get; set; } = new();
            
            [JsonExtensionData]
            public Dictionary<string, JsonElement>? ExtensionData { get; set; }
        }

        // Task configuration classes
        public class TaskConfiguration
        {
            [JsonPropertyName("label")]
            public string Label { get; set; } = "";
            
            [JsonPropertyName("type")]
            public string Type { get; set; } = "shell";
            
            [JsonPropertyName("command")]
            public string Command { get; set; } = "";
            
            [JsonPropertyName("args")]
            public List<string> Args { get; set; } = new();
            
            [JsonPropertyName("group")]
            [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
            public TaskGroup? Group { get; set; }
            
            [JsonPropertyName("problemMatcher")]
            public string ProblemMatcher { get; set; } = "$msCompile";
            
            [JsonPropertyName("presentation")]
            public TaskPresentation Presentation { get; set; } = new();
        }
        
        public class TaskGroup
        {
            [JsonPropertyName("kind")]
            public string Kind { get; set; } = "build";
            
            [JsonPropertyName("isDefault")]
            public bool IsDefault { get; set; } = false;
        }
        
        public class TaskPresentation
        {
            [JsonPropertyName("echo")]
            public bool Echo { get; set; } = true;
            
            [JsonPropertyName("reveal")]
            public string Reveal { get; set; } = "always";
            
            [JsonPropertyName("focus")]
            public bool Focus { get; set; } = false;
            
            [JsonPropertyName("panel")]
            public string Panel { get; set; } = "shared";
            
            [JsonPropertyName("showReuseMessage")]
            public bool ShowReuseMessage { get; set; } = true;
            
            [JsonPropertyName("clear")]
            public bool Clear { get; set; } = false;
        }

        // Base configuration with common properties
        public class DebugConfigurationBase
        {
            [JsonPropertyName("name")]
            public string Name { get; set; } = "";
            
            [JsonPropertyName("type")]
            public string Type { get; set; } = "";
            
            [JsonPropertyName("request")]
            public string Request { get; set; } = "launch";
            
            [JsonPropertyName("program")]
            public string Program { get; set; } = "";
            
            [JsonPropertyName("args")]
            [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingDefault)]
            public List<string>? Args { get; set; }
            
            [JsonPropertyName("cwd")]
            public string Cwd { get; set; } = "";
            
            [JsonPropertyName("stopAtEntry")]
            [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingDefault)]
            public bool StopAtEntry { get; set; } = false;
            
            [JsonPropertyName("stopOnEntry")]
            [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingDefault)]
            public bool StopOnEntry { get; set; } = false;
            
            [JsonPropertyName("preLaunchTask")]
            [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
            public string? PreLaunchTask { get; set; }
            
            [JsonPropertyName("console")]
            [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
            public string? Console { get; set; }
        }

        // Visual Studio debugger configuration
        public class VSDebugConfiguration : DebugConfigurationBase
        {
            [JsonConstructor]
            public VSDebugConfiguration()
            {
                Type = "cppvsdbg";
                Console = "integratedTerminal";
            }
        }

        // LLDB debugger configuration
        public class LLDBDebugConfiguration : DebugConfigurationBase
        {
            [JsonPropertyName("sourceMap")]
            [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
            public List<List<string>>? SourceMap { get; set; }
            
            [JsonPropertyName("env")]
            [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
            public Dictionary<string, string>? Env { get; set; }
            
            [JsonPropertyName("runInTerminal")]
            [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingDefault)]
            public bool RunInTerminal { get; set; } = false;
            
            [JsonConstructor]
            public LLDBDebugConfiguration() { }
            
            public LLDBDebugConfiguration(bool useDap) : this()
            {
                Type = useDap ? "lldb-dap" : "lldb";
                if (useDap)
                {
                    // lldb-dap 使用 stopOnEntry 而不是 stopAtEntry
                    StopOnEntry = StopAtEntry;
                    StopAtEntry = false; // 避免序列化这个属性
                    RunInTerminal = true; // lldb-dap 默认在终端运行
                }
                else
                {
                    // CodeLLDB 支持 console 属性
                    Console = "integratedTerminal";
                }
            }
        }

        // GDB debugger configuration
        public class GDBDebugConfiguration : DebugConfigurationBase
        {
            [JsonPropertyName("MIMode")]
            public string MIMode { get; set; } = "gdb";
            
            [JsonPropertyName("miDebuggerPath")]
            public string MIDebuggerPath { get; set; } = "gdb";
            
            [JsonPropertyName("setupCommands")]
            [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
            public List<object>? SetupCommands { get; set; }
            
            [JsonConstructor]
            public GDBDebugConfiguration()
            {
                Type = "cppdbg";
            }
        }

        private static readonly ConcurrentDictionary<string, List<DebugConfigurationBase>> TargetConfigurations = new();
        
        public static string WorkspaceRoot { get; set; } = "./";
        public static string VSCodeDirectory { get; set; } = ".vscode";
        public static bool PreserveUserConfigurations { get; set; } = true;
        public static DebuggerType DefaultDebugger { get; set; } = DebuggerType.CppDbg;

        private IToolchain Toolchain { get; }
        
        public VSCodeDebugEmitter(IToolchain Toolchain) => this.Toolchain = Toolchain;

        public override bool EnableEmitter(Target Target) => Target.GetTargetType() == TargetType.Executable;
        public override bool EmitTargetTask(Target Target) => true;

        public override IArtifact? PerTargetTask(Target Target)
        {
            if (!EnableEmitter(Target)) return null;

            var configurations = new[] { "Debug", "Release", "RelWithDebInfo" }
                .Select(buildType => CreateDebugConfiguration(Target, buildType))
                .ToList();

            TargetConfigurations.TryAdd(Target.Name, configurations);
            return null;
        }

        public static void GenerateDebugConfigurations()
        {
            GenerateLaunchJson();
            GenerateTasksJson();
        }

        private DebugConfigurationBase CreateDebugConfiguration(Target target, string buildType)
        {
            var exeName = target.Name + (BS.TargetOS == OSPlatform.Windows ? ".exe" : "");
            var binaryPath = target.GetBinaryPath();
            var relativeBinaryPath = Path.GetRelativePath(WorkspaceRoot, binaryPath);

            var config = SelectDebugger();
            config.Name = $"{target.Name} ({buildType})";
            config.Program = Path.Combine("${workspaceFolder}", relativeBinaryPath, exeName);
            config.Cwd = Path.Combine("${workspaceFolder}", relativeBinaryPath);
            config.PreLaunchTask = $"Build {target.Name} ({buildType})";

            if (target.Arguments.TryGetValue("RunArgs", out var runArgs) && runArgs is List<string> argsList && argsList.Count > 0)
                config.Args = argsList;

            return config;

            DebugConfigurationBase SelectDebugger() => (DefaultDebugger, Toolchain, BS.TargetOS) switch
            {
                // 自动选择最佳调试器
                (DebuggerType.CppDbg, VisualStudio, OSPlatform.Windows) => new VSDebugConfiguration(),
                (DebuggerType.CppDbg, XCode, _) => new LLDBDebugConfiguration(false),
                
                // 用户指定的调试器
                (DebuggerType.CppVsDbg, _, OSPlatform.Windows) => new VSDebugConfiguration(),
                (DebuggerType.LLDBDap, _, _) => new LLDBDebugConfiguration(true),
                (DebuggerType.CodeLLDB, _, _) => new LLDBDebugConfiguration(false),
                
                // 默认 GDB
                _ => new GDBDebugConfiguration 
                { 
                    MIDebuggerPath = FindDebugger("gdb")!,
                    SetupCommands = BS.TargetOS == OSPlatform.Linux ? new List<object>
                    {
                        new { description = "Enable pretty-printing", text = "-enable-pretty-printing", ignoreFailures = true }
                    } : null
                }
            };
        }

        private static void GenerateLaunchJson()
        {
            var path = Path.Combine(WorkspaceRoot, VSCodeDirectory, "launch.json");
            var launchJson = LoadOrCreateLaunchJson(path);

            launchJson.Configurations.RemoveAll(IsGeneratedConfig);
            
            // Sort by target name first, then flatten the configurations
            var generatedConfigs = TargetConfigurations
                .OrderBy(kv => kv.Key)  // Sort by target name
                .SelectMany(kv => kv.Value)
                .Select(c => { c.Name = $"[SB Generated] {c.Name}"; return c; })
                .ToList();

            launchJson.Configurations.AddRange(generatedConfigs);

            WriteJsonFile(path, launchJson);
            Log.Information($"Generated VSCode launch.json with {launchJson.Configurations.Count} configurations");
        }

        private static void GenerateTasksJson()
        {
            var path = Path.Combine(WorkspaceRoot, VSCodeDirectory, "tasks.json");
            var tasksJson = LoadOrCreateTasksJson(path);

            tasksJson.Tasks.RemoveAll(IsGeneratedTask);

            // Sort by target name first, then create tasks
            var tasks = (from entry in TargetConfigurations.OrderBy(kv => kv.Key)
                        from config in entry.Value
                        let buildType = ExtractBuildType(config.Name)
                        select CreateBuildTask(entry.Key, buildType, config.PreLaunchTask!))
                        .ToList();

            tasksJson.Tasks.AddRange(tasks);

            WriteJsonFile(path, tasksJson);
            Log.Information($"Generated VSCode tasks.json with {tasksJson.Tasks.Count} tasks");
        }
        
        private static bool IsGeneratedConfig(DebugConfigurationBase c) => c.Name.Contains("[SB Generated]");
        private static bool IsGeneratedTask(TaskConfiguration t) => t.Label.StartsWith("Build ") && 
            TargetConfigurations.Keys.Any(target => t.Label.Contains(target));
        
        private static string ExtractBuildType(string configName) => configName switch
        {
            var n when n.Contains("Debug") => "Debug",
            var n when n.Contains("Release") => "Release",
            _ => "RelWithDebInfo"
        };
        
        private static TaskConfiguration CreateBuildTask(string targetName, string buildType, string label) => new()
        {
            Label = label,
            Command = "dotnet",
            Args = new List<string> { "run", "SB", "--", "build", "--target", targetName, "--mode", buildType.ToLower() },
            Group = new TaskGroup { Kind = "build", IsDefault = buildType == "Debug" }
        };

        private static LaunchJson LoadOrCreateLaunchJson(string path) => LoadOrCreateJson<LaunchJson>(path, true);
        private static TasksJson LoadOrCreateTasksJson(string path) => LoadOrCreateJson<TasksJson>(path, false);
        
        private static T LoadOrCreateJson<T>(string path, bool useConverter) where T : new()
        {
            if (PreserveUserConfigurations && File.Exists(path))
            {
                try
                {
                    var options = new JsonSerializerOptions { PropertyNameCaseInsensitive = true };
                    if (useConverter) options.Converters.Add(new PolymorphicDebugConfigurationConverter());
                    return JsonSerializer.Deserialize<T>(File.ReadAllText(path), options) ?? new T();
                }
                catch (Exception ex)
                {
                    Log.Warning($"Failed to parse existing {Path.GetFileName(path)}: {ex.Message}. Creating new file.");
                }
            }
            return new T();
        }

        private static void WriteJsonFile<T>(string path, T content)
        {
            Directory.CreateDirectory(Path.GetDirectoryName(path)!);
            
            var options = new JsonSerializerOptions
            {
                WriteIndented = true,
                DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull,
                Converters = { new PolymorphicDebugConfigurationConverter() }
            };

            File.WriteAllText(path, JsonSerializer.Serialize(content, options));
        }

        private static string? FindDebugger(string debuggerName)
        {
            // 在 Windows 上添加 .exe 扩展名
            if (BS.HostOS == OSPlatform.Windows && !debuggerName.EndsWith(".exe"))
                debuggerName += ".exe";
                
            var paths = Environment.GetEnvironmentVariable("PATH")?.Split(Path.PathSeparator) ?? Array.Empty<string>();
            var found = paths.Select(p => Path.Combine(p, debuggerName)).FirstOrDefault(File.Exists);
            
            // 如果找到了，返回完整路径；否则返回原始名称让系统处理
            return found ?? debuggerName;
        }
        
        // Custom JSON converter for polymorphic debug configurations
        private class PolymorphicDebugConfigurationConverter : JsonConverter<DebugConfigurationBase>
        {
            public override DebugConfigurationBase? Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
            {
                using var doc = JsonDocument.ParseValue(ref reader);
                var root = doc.RootElement;
                
                if (!root.TryGetProperty("type", out var typeElement)) return null;
                
                var json = root.GetRawText();
                return typeElement.GetString() switch
                {
                    "cppvsdbg" => JsonSerializer.Deserialize<VSDebugConfiguration>(json, options),
                    "lldb" or "lldb-dap" => JsonSerializer.Deserialize<LLDBDebugConfiguration>(json, options),
                    "cppdbg" => JsonSerializer.Deserialize<GDBDebugConfiguration>(json, options),
                    _ => JsonSerializer.Deserialize<DebugConfigurationBase>(json, options)
                };
            }
            
            public override void Write(Utf8JsonWriter writer, DebugConfigurationBase value, JsonSerializerOptions options) => 
                JsonSerializer.Serialize(writer, value, value.GetType(), options);
        }
    }
}