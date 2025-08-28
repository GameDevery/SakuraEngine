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
using System.Reflection.Emit;
using Microsoft.VisualBasic;
using System.Text.RegularExpressions;

// Reference:
//  - https://code.visualstudio.com/docs/debugtest/tasks
//  - https://code.visualstudio.com/docs/cpp/launch-json-reference#_processid


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

            [JsonPropertyName("dependsOn")]
            [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
            public List<string>? DependsOn { get; set; }
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
                    RunInTerminal = false;
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

        public static void GenerateDebugConfigurations(IToolchain toolchain)
        {
            GenerateLaunchJson();
            GenerateTasksJson(toolchain);
        }

        private DebugConfigurationBase CreateDebugConfiguration(Target target, string buildType)
        {
            var exeName = target.Name + (BS.TargetOS == OSPlatform.Windows ? ".exe" : "");
            var binaryPath = target.GetBinaryPath(buildType).ToLowerInvariant();
            var relativeBinaryPath = Path.GetRelativePath(WorkspaceRoot, binaryPath);

            var config = SelectDebugger();
            config.Name = $"{target.Name} ({buildType})";
            config.Program = Path.Combine("${workspaceFolder}", relativeBinaryPath, exeName);
            config.Cwd = Path.Combine("${workspaceFolder}", relativeBinaryPath);
            config.PreLaunchTask = $"Build {target.Name} ({buildType})";

            // Support for RunArgs (backwards compatibility)
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

        private static void GenerateTasksJson(IToolchain toolchain)
        {
            var path = Path.Combine(WorkspaceRoot, VSCodeDirectory, "tasks.json");
            var tasksJson = LoadOrCreateTasksJson(path);

            tasksJson.Tasks.RemoveAll(IsGeneratedTask);

            var allTasks = new List<TaskConfiguration>();

            // Sort by target name first, then create tasks
            foreach (var entry in TargetConfigurations.OrderBy(kv => kv.Key))
            {
                var targetName = entry.Key;
                var target = BS.GetTarget(targetName);

                foreach (var config in entry.Value)
                {
                    var buildType = ExtractBuildType(config.Name);
                    var dependencies = new List<string>();

                    // Create the build task
                    var buildTask = CreateBuildTask(targetName, buildType, config.PreLaunchTask!, toolchain.Name);
                    if (dependencies.Count > 0)
                    {
                        buildTask.DependsOn = dependencies;
                    }
                    allTasks.Add(buildTask);
                }
            }

            tasksJson.Tasks.AddRange(allTasks);

            WriteJsonFile(path, tasksJson);
            Log.Information($"Generated VSCode tasks.json with {tasksJson.Tasks.Count} tasks");
        }

        private static bool IsGeneratedConfig(DebugConfigurationBase c) => c.Name.Contains("[SB Generated]");
        private static bool IsGeneratedTask(TaskConfiguration t) =>
            (t.Label.StartsWith("Build ") || t.Label.StartsWith("PreRun ")) &&
            TargetConfigurations.Keys.Any(target => t.Label.Contains(target));

        private static string ExtractBuildType(string configName) => configName switch
        {
            var n when n.Contains("Debug") => "Debug",
            var n when n.Contains("Release") => "Release",
            _ => "RelWithDebInfo"
        };

        private static TaskConfiguration CreateBuildTask(string targetName, string buildType, string label, string toolchain) => new()
        {
            Label = label,
            Command = "dotnet",
            Args = new List<string> { "run", "SB", "--", "build", "--target", targetName, "--mode", buildType.ToLower(), "--toolchain", toolchain },
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

    public class VSCodeDebugEmitterNew : TaskEmitter
    {
        // configs
        public string Mode = "Debug"; // build modes to filter generated configs
        public string Debugger = "";
        public IToolchain? Toolchain = null;
        public bool PreserveUserConfig = true;

        // path
        public string CmdFilesOutputDir = "";
        public string MergedNatvisOutputDir = "";
        public string WorkspaceRoot = "";

        // targets to resolve
        private ConcurrentBag<Target> _TargetsToGenerate = new();

        // cache
        private Dictionary<Target, JsonObject> _PerTargetPreLaunchTasks = new();
        private List<JsonObject> _LaunchConfigs = new();

        // emit tasks
        public override bool EnableEmitter(Target Target) => Target.GetTargetType() == TargetType.Executable;
        public override bool EmitTargetTask(Target Target) => true;
        public override IArtifact? PerTargetTask(Target Target)
        {
            _TargetsToGenerate.Add(Target);
            return null;
        }

        // generate
        public void Check()
        {
            // check config
            if (Toolchain == null)
            {
                throw new InvalidOperationException("Toolchain is not set for VSCodeDebugEmitter");
            }

            // check path
            if (string.IsNullOrEmpty(CmdFilesOutputDir))
            {
                throw new InvalidOperationException("CmdFilesOutputDir is not set for VSCodeDebugEmitter");
            }
            if (string.IsNullOrEmpty(MergedNatvisOutputDir))
            {
                throw new InvalidOperationException("MergedNatvisOutputDir is not set for VSCodeDebugEmitter");
            }
            if (string.IsNullOrEmpty(WorkspaceRoot))
            {
                throw new InvalidOperationException("WorkspaceRoot is not set for VSCodeDebugEmitter");
            }
        }
        public void Generate()
        {
            Check();

            if (string.IsNullOrEmpty(Debugger))
            {
                Debugger = _GetDefaultDebugger();
            }

            // sort targets
            _TargetsToGenerate = new(_TargetsToGenerate.OrderByDescending(t => t.Name));

            // collect tasks
            foreach (var target in _TargetsToGenerate)
            {
                // get custom pre run cmds
                var preRunCmds = TargetExtensions.DebugPreRunCmdDic.TryGetValue(target, out var cmds) ? cmds : new List<string>();

                // insert build cmd
                preRunCmds.Insert(0, _MakeBuildCmd(target, Mode, Toolchain!.Name));

                // make task json
                var taskJson = _MakeTaskJson(preRunCmds, $"PreRun_{target.Name}_{Mode}_{Toolchain!.Name}");

                // cache task json
                if (taskJson != null)
                    _PerTargetPreLaunchTasks[target] = taskJson;
            }

            // collect launches
            foreach (var target in _TargetsToGenerate)
            {
                // make launch json
                var launchJson = _MakeLaunchJson(target);

                // cache launch json
                _LaunchConfigs.Add(launchJson);
            }

            // generate tasks.json
            {
                // get path
                var taskJsonPath = Path.Combine(WorkspaceRoot, ".vscode", "tasks.json");
                if (!Directory.Exists(Path.GetDirectoryName(taskJsonPath)!))
                    Directory.CreateDirectory(Path.GetDirectoryName(taskJsonPath)!);

                JsonObject? taskJsonObj = null;

                // try load existing tasks.json
                if (File.Exists(taskJsonPath) && PreserveUserConfig)
                {
                    // load json object
                    var jsonText = File.ReadAllText(taskJsonPath);
                    taskJsonObj = JsonNode.Parse(jsonText)!.AsObject();

                    // remove tasks we generated before
                    if (taskJsonObj.ContainsKey("tasks") && taskJsonObj["tasks"] is JsonArray tasksArray)
                    {
                        _RemoveObjWithTag(tasksArray);
                    }
                }

                if (taskJsonObj == null)
                {
                    taskJsonObj = new JsonObject
                    {
                        ["version"] = "2.0.0",
                        ["tasks"] = JsonSerializer.SerializeToNode(_PerTargetPreLaunchTasks.Values)
                    };
                }
                else
                {
                    // append new tasks
                    foreach (var task in _PerTargetPreLaunchTasks.Values)
                    {
                        if (taskJsonObj["tasks"] is JsonArray tasksArray)
                        {
                            tasksArray.Add(task);
                        }
                    }
                }

                // write to file
                File.WriteAllText(taskJsonPath, taskJsonObj.ToJsonString(new JsonSerializerOptions
                {
                    TypeInfoResolver = new System.Text.Json.Serialization.Metadata.DefaultJsonTypeInfoResolver(),
                    WriteIndented = true,
                    // DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull
                }));
            }

            // generate launch.json
            {
                // get path
                var launchJsonPath = Path.Combine(WorkspaceRoot, ".vscode", "launch.json");
                if (!Directory.Exists(Path.GetDirectoryName(launchJsonPath)!))
                    Directory.CreateDirectory(Path.GetDirectoryName(launchJsonPath)!);

                JsonObject? launchJsonObj = null;

                // try load existing launch.json
                if (File.Exists(launchJsonPath) && PreserveUserConfig)
                {
                    // load json object
                    var jsonText = File.ReadAllText(launchJsonPath);
                    launchJsonObj = JsonNode.Parse(jsonText)!.AsObject();

                    // remove configs we generated before
                    if (launchJsonObj.ContainsKey("configurations") && launchJsonObj["configurations"] is JsonArray configsArray)
                    {
                        _RemoveObjWithTag(configsArray);
                    }
                }

                if (launchJsonObj == null)
                {
                    launchJsonObj = new JsonObject
                    {
                        ["version"] = "0.2.0",
                        ["configurations"] = JsonSerializer.SerializeToNode(_LaunchConfigs)
                    };
                }
                else
                {
                    // append new configs
                    foreach (var config in _LaunchConfigs)
                    {
                        if (launchJsonObj["configurations"] is JsonArray configsArray)
                        {
                            configsArray.Add(config);
                        }
                    }
                }

                // write to file
                File.WriteAllText(launchJsonPath, launchJsonObj.ToJsonString(new JsonSerializerOptions
                {
                    TypeInfoResolver = new System.Text.Json.Serialization.Metadata.DefaultJsonTypeInfoResolver(),
                    WriteIndented = true,
                    // DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull
                }));
            }
        }
        public void Clear()
        {
            Check();

            // remove task.json
            {
                // get path
                var taskJsonPath = Path.Combine(WorkspaceRoot, ".vscode", "tasks.json");
                if (!Directory.Exists(Path.GetDirectoryName(taskJsonPath)!))
                    Directory.CreateDirectory(Path.GetDirectoryName(taskJsonPath)!);
                if (File.Exists(taskJsonPath) && PreserveUserConfig)
                {
                    // load json object
                    var jsonText = File.ReadAllText(taskJsonPath);
                    var taskJsonObj = JsonNode.Parse(jsonText)!.AsObject();

                    // remove tasks we generated before
                    if (taskJsonObj.ContainsKey("tasks") && taskJsonObj["tasks"] is JsonArray tasksArray)
                    {
                        _RemoveObjWithTag(tasksArray);
                    }

                    // write to file
                    File.WriteAllText(taskJsonPath, taskJsonObj.ToJsonString(new JsonSerializerOptions
                    {
                        TypeInfoResolver = new System.Text.Json.Serialization.Metadata.DefaultJsonTypeInfoResolver(),
                        WriteIndented = true,
                        // DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull
                    }));
                }
            }

            // remove launch.json
            {
                // get path
                var launchJsonPath = Path.Combine(WorkspaceRoot, ".vscode", "launch.json");
                if (!Directory.Exists(Path.GetDirectoryName(launchJsonPath)!))
                    Directory.CreateDirectory(Path.GetDirectoryName(launchJsonPath)!);
                if (File.Exists(launchJsonPath) && PreserveUserConfig)
                {
                    // load json object
                    var jsonText = File.ReadAllText(launchJsonPath);
                    var launchJsonObj = JsonNode.Parse(jsonText)!.AsObject();

                    // remove configs we generated before
                    if (launchJsonObj.ContainsKey("configurations") && launchJsonObj["configurations"] is JsonArray configsArray)
                    {
                        _RemoveObjWithTag(configsArray);
                    }

                    // write to file
                    File.WriteAllText(launchJsonPath, launchJsonObj.ToJsonString(new JsonSerializerOptions
                    {
                        TypeInfoResolver = new System.Text.Json.Serialization.Metadata.DefaultJsonTypeInfoResolver(),
                        WriteIndented = true,
                        // DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull
                    }));
                }
            }

            // remove cmd files
            if (Directory.Exists(CmdFilesOutputDir))
            {
                Directory.Delete(CmdFilesOutputDir, true);
            }

            // remove natvis files
            if (Directory.Exists(MergedNatvisOutputDir))
            {
                Directory.Delete(MergedNatvisOutputDir, true);
            }
        }


        // helpers
        private void _RemoveObjWithTag(JsonArray arr)
        {
            // search obj with __SB_TAG__
            var toRemove = new List<JsonNode>();
            foreach (var task in arr)
            {
                if (task is JsonObject taskObj && taskObj.ContainsKey("__SB_TAG__") && taskObj["__SB_TAG__"]?.GetValue<bool>() == true)
                {
                    toRemove.Add(task);
                }
            }

            // remove them
            foreach (var task in toRemove)
            {
                arr.Remove(task);
            }
        }
        private string? _GenerateCmdFile(List<string> cmds, string cmdName)
        {
            if (cmds.Count >= 0)
            {
                // build cmd file content
                var sb = new StringBuilder();
                foreach (var cmd in cmds)
                {
                    // push cmd
                    sb.AppendLine(cmd);

                    // push error break
                    sb.AppendLine(BS.HostOS switch
                    {
                        OSPlatform.Windows => "IF %ERRORLEVEL% NEQ 0 (exit %ERRORLEVEL%)",
                        OSPlatform.Linux or OSPlatform.OSX => "if [ $? -ne 0 ]; then exit $?; fi",
                        _ => throw new NotSupportedException($"Unsupported OS: {BS.HostOS}"),
                    });
                }

                // get cmd file suffix
                var cmdFileExt = BS.HostOS switch
                {
                    OSPlatform.Windows => ".bat",
                    OSPlatform.Linux or OSPlatform.OSX => ".sh",
                    _ => throw new NotSupportedException($"Unsupported OS: {BS.HostOS}"),
                };

                // solve output path
                var outputPath = Path.Combine(CmdFilesOutputDir, $"{cmdName}{cmdFileExt}");

                // try create dir first
                if (!Directory.Exists(CmdFilesOutputDir))
                    Directory.CreateDirectory(CmdFilesOutputDir);

                // write to file
                File.WriteAllText(outputPath, sb.ToString());
                return outputPath;
            }
            return null;
        }
        private JsonObject? _MakeTaskJson(List<string> cmds, string cmdName)
        {
            var cmdFilePath = _GenerateCmdFile(cmds, cmdName);
            if (cmdFilePath != null)
            {
                return new JsonObject
                {
                    ["__SB_TAG__"] = true,
                    ["label"] = cmdName,
                    ["type"] = "shell",
                    ["command"] = BS.HostOS switch
                    {
                        OSPlatform.Windows => cmdFilePath,
                        OSPlatform.OSX or OSPlatform.Linux => $"chmod u+x {cmdFilePath} && {cmdFilePath}",
                        _ => throw new NotSupportedException($"Unsupported OS: {BS.HostOS}"),
                    },
                    ["options"] = new JsonObject
                    {
                        ["cwd"] = "${workspaceFolder}"
                    },
                    ["group"] = new JsonObject
                    {
                        ["kind"] = "build",
                        ["isDefault"] = false
                    },
                    ["presentation"] = new JsonObject
                    {
                        ["echo"] = true,
                        ["reveal"] = "always",
                        ["focus"] = false,
                        ["panel"] = "shared",
                        ["showReuseMessage"] = true,
                        ["clear"] = false,
                        ["close"] = true,
                    }
                };
            }
            return null;
        }
        private JsonObject _MakeLaunchJson(Target target)
        {
            // try dump natvis file
            string natvisFileName = "";
            {
                var natvisFiles = _SolveNatvisFiles(target);
                if (natvisFiles.Count != 0)
                {
                    var mergedNatvis = MiscTools.MergeNatvis(natvisFiles);

                    if (Directory.Exists(MergedNatvisOutputDir) == false)
                        Directory.CreateDirectory(MergedNatvisOutputDir);

                    natvisFileName = Path.Combine(MergedNatvisOutputDir, $"{target.Name}.natvis");
                    File.WriteAllText(natvisFileName, mergedNatvis);
                }
            }

            // solve target info
            var args = TargetExtensions.DebugArgsDic.TryGetValue(target, out var a) ? a : new List<string>();

            var result = new JsonObject
            {
                ["__SB_TAG__"] = true,
                ["name"] = $"▶️{target.Name} [{Mode}]",
                ["type"] = Debugger,
                ["request"] = "launch",
                ["program"] = _GetTargetProgramPath(target),
                ["args"] = args.Count > 0 ? JsonSerializer.SerializeToNode(args) : new JsonArray(),
                ["cwd"] = _GetTargetProgramDir(target),
                ["stopAtEntry"] = false,
                ["console"] = "integratedTerminal",
                ["visualizerFile"] = Path.Join("${workspaceFolder}", natvisFileName),
                ["preLaunchTask"] = _PerTargetPreLaunchTasks[target]?["label"]?.GetValue<string>(),
                ["autoAttachChildProcess"] = true,
            };

            if (Debugger == "lldb" || Debugger == "lldb-dap")
            {
                result["sourceMap"] = null;
                result["env"] = null;
                result["runInTerminal"] = false;
                if (Debugger == "lldb-dap")
                {
                    result["stopOnEntry"] = false;
                }
            }
            else if (Debugger == "cppdbg")
            {
                result["MIMode"] = "gdb";
                result["miDebuggerPath"] = _FindDebugger("gdb");
                result["setupCommands"] = BS.TargetOS == OSPlatform.Linux ? new JsonArray
                {
                    new JsonObject
                    {
                        ["description"] = "Enable pretty-printing",
                        ["text"] = "-enable-pretty-printing",
                        ["ignoreFailures"] = true
                    }
                } : null;
            }

            return result;
        }
        private static string? _FindDebugger(string debuggerName)
        {
            // 在 Windows 上添加 .exe 扩展名
            if (BS.HostOS == OSPlatform.Windows && !debuggerName.EndsWith(".exe"))
                debuggerName += ".exe";

            var paths = Environment.GetEnvironmentVariable("PATH")?.Split(Path.PathSeparator) ?? Array.Empty<string>();
            var found = paths.Select(p => Path.Combine(p, debuggerName)).FirstOrDefault(File.Exists);

            // 如果找到了，返回完整路径；否则返回原始名称让系统处理
            return found ?? debuggerName;
        }
        private string _GetTargetProgramPath(Target target)
        {
            var exeName = target.Name + (BS.TargetOS == OSPlatform.Windows ? ".exe" : "");
            return Path.Combine(_GetTargetProgramDir(target), exeName);
        }
        private string _GetTargetProgramDir(Target target)
        {
            var binaryPath = target.GetBinaryPath(Mode).ToLowerInvariant();
            var relativeBinaryPath = Path.GetRelativePath(WorkspaceRoot, binaryPath);
            return Path.Combine("${workspaceFolder}", relativeBinaryPath);
        }
        // cppvsdbg, cppdbg, lldb-dap, lldb
        private string _GetDefaultDebugger()
        {
            switch (BS.TargetOS)
            {
                case OSPlatform.Windows:
                    return "cppvsdbg";
                case OSPlatform.Linux:
                case OSPlatform.OSX:
                    return "cppdbg"; // gdb
                default:
                    return "cppdbg";
            }
        }
        private List<string> _SolveNatvisFiles(Target target)
        {
            var files = new List<string>();

            var _AddNatvisFileForTarget = (Target t) =>
            {
                var natvisList = t.FileList<NatvisFileList>();
                if (natvisList != null)
                {
                    foreach (var f in natvisList.Files)
                    {
                        if (File.Exists(f))
                            files.Add(Path.GetFullPath(f));
                        else
                            Log.Warning($"Natvis file '{f}' not found for target '{t.Name}'");
                    }
                }
            };

            // collect this target natvis files
            _AddNatvisFileForTarget(target);

            // collect dependencies natvis files
            foreach (var depName in target.Dependencies)
            {
                var depTarget = BS.GetTarget(depName);
                if (depTarget != null)
                {
                    _AddNatvisFileForTarget(depTarget);
                }
                else
                {
                    Log.Warning($"Dependency target '{depName}' not found for target '{target.Name}'");
                }
            }

            return files;
        }
        private string _NormalizeCmdName(string label)
        {
            return Regex.Replace(label, @"[^\w]", "_");
        }
        private string _MakeBuildCmd(Target target, string mode, string toolchain)
        {
            return $"dotnet run SB -- build --target {target.Name} --mode {mode.ToLower()} --toolchain {toolchain}";
        }
    }


    public class NatvisFileList : FileList
    {
    }

    public static partial class TargetExtensions
    {
        public static Target DebugPreRunCmd(this Target @this, string cmd)
        {
            if (!DebugPreRunCmdDic.TryGetValue(@this, out var preRuns))
                DebugPreRunCmdDic.Add(@this, new());
            DebugPreRunCmdDic[@this].Add(cmd);
            return @this;
        }
        public static Target DebugArgs(this Target @this, params string[] args)
        {
            DebugArgsDic[@this] = args.ToList();
            return @this;
        }
        public static Target AddNatvisFiles(this Target @this, params string[] files)
        {
            @this.FileList<NatvisFileList>().AddFiles(files);
            return @this;
        }


        internal static Dictionary<Target, List<string>> DebugPreRunCmdDic = new();
        internal static Dictionary<Target, List<string>> DebugArgsDic = new();
    }
}