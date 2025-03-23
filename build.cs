using SB;
using SB.Core;
using Serilog;
using System.Diagnostics;

Stopwatch before_sw = new();
before_sw.Start();

var Toolchain = Engine.Bootstrap(SourceLocation.Directory());

// TEMP HELPER CODE, REMOVE LATER
static void ScanDirectories(string currentPath, List<string> result)
{
    // 检查当前目录的文件状态 
    bool hasXmake = File.Exists(Path.Combine(currentPath, "xmake.lua"));
    bool hasBuildCs = File.Exists(Path.Combine(currentPath, "build.cs"));

    // 符合条件则记录 
    if (hasXmake && !hasBuildCs)
    {
        result.Add(currentPath);
    }

    // 递归处理子目录 
    foreach (string subDir in Directory.EnumerateDirectories(currentPath))
    {
        ScanDirectories(subDir, result);
    }
}
List<string> DIRS = new();
ScanDirectories(SourceLocation.Directory(), DIRS);
DIRS.ForEach(D => Log.Information($"Found xmake.lua in {D}"));
// TEMP HELPER CODE, REMOVE LATER

Engine.AddTaskEmitter("ModuleMeta", new ModuleMetaEmitter());

Engine.AddTaskEmitter("Codgen.Meta", new CodegenMetaEmitter(Toolchain));

Engine.AddTaskEmitter("Codgen.Codegen", new CodegenRenderEmitter(Toolchain))
    .AddDependency("Codgen.Meta", DependencyModel.ExternalTarget)
    .AddDependency("Codgen.Meta", DependencyModel.PerTarget);

Engine.AddTaskEmitter("Cpp.UnityBuild", new UnityBuildEmitter())
    .AddDependency("ModuleMeta", DependencyModel.PerTarget)
    .AddDependency("Codgen.Codegen", DependencyModel.PerTarget);

Engine.AddTaskEmitter("Cpp.Compile", new CppCompileEmitter(Toolchain))
    .AddDependency("ModuleMeta", DependencyModel.PerTarget)
    .AddDependency("Cpp.UnityBuild", DependencyModel.PerTarget)
    .AddDependency("Codgen.Codegen", DependencyModel.ExternalTarget)
    .AddDependency("Codgen.Codegen", DependencyModel.PerTarget);

var CompileCommandsEmitter = new CompileCommandsEmitter(Toolchain);
Engine.AddTaskEmitter("Cpp.CompileCommands", CompileCommandsEmitter)
    .AddDependency("ModuleMeta", DependencyModel.PerTarget)
    .AddDependency("Codgen.Codegen", DependencyModel.ExternalTarget)
    .AddDependency("Codgen.Codegen", DependencyModel.PerTarget);

Engine.AddTaskEmitter("Cpp.Link", new CppLinkEmitter(Toolchain))
    .AddDependency("Cpp.Link", DependencyModel.ExternalTarget)
    .AddDependency("Cpp.Compile", DependencyModel.PerTarget);

Engine.Module("TestTarget")
    .TargetType(TargetType.Static)

    .CppVersion("20")

    .Depend(Visibility.Public, "SkrCore")

    .Require("imgui", new ImGuiPackageConfig { Version = new Version(1, 89, 0), ImportDynamicAPIFromEngine = true })
    .Depend(Visibility.Private, "imgui@imgui")

    .Require("Luau", new PackageConfig { Version = new Version(0, 613, 1) })
    .Depend(Visibility.Private, "Luau@VM")
    .Depend(Visibility.Private, "Luau@Ast")
    .Depend(Visibility.Private, "Luau@Compiler")

    .Require("lua", new PackageConfig { Version = new Version(5, 4, 4) })
    .Depend(Visibility.Private, "lua@lua")

    .Require("lmdb", new PackageConfig { Version = new Version(0, 9, 29) })
    .Depend(Visibility.Private, "lmdb@lmdb")

    .Require("tinygltf", new PackageConfig { Version = new Version(2, 8, 14) })
    .Depend(Visibility.Private, "tinygltf@tinygltf")

    .Require("nanovg", new PackageConfig { Version = new Version(0, 1, 0) })
    .Depend(Visibility.Private, "nanovg@nanovg")

    .Require("wasm3", new PackageConfig { Version = new Version(0, 5, 0) })
    .Depend(Visibility.Private, "wasm3@wasm3")

    .Require("lemon", new PackageConfig { Version = new Version(1, 3, 1) })
    .Depend(Visibility.Private, "lemon@lemon")

    .Require("MeshOptimizer", new PackageConfig { Version = new Version(0, 0, 1) })
    .Depend(Visibility.Private, "MeshOptimizer@MeshOptimizer")

    .Require("zlib", new PackageConfig { Version = new Version(1, 2, 8) })
    .Depend(Visibility.Private, "zlib@zlib")

    .Require("cgltf", new PackageConfig { Version = new Version(1, 13, 0) })
    .Depend(Visibility.Private, "cgltf@cgltf")

    .Require("cpu_features", new PackageConfig { Version = new Version(0, 9, 0) })
    .Depend(Visibility.Private, "cpu_features@cpu_features")

    .Require("yyjson", new PackageConfig { Version = new Version(0, 9, 0) })
    .Depend(Visibility.Private, "yyjson@yyjson")

    .Require("DaScriptCore", new PackageConfig { Version = new Version(0, 9, 0) })
    .Depend(Visibility.Private, "DaScriptCore@DaScriptCore")

    .Require("harfbuzz", new PackageConfig { Version = new Version(7, 1, 0) })
    .Depend(Visibility.Private, "harfbuzz@harfbuzz");

before_sw.Stop();

Stopwatch sw = new();
sw.Start();

Engine.RunBuild();
CompileCommandsEmitter.WriteToFile(Path.Combine(SourceLocation.Directory(), ".vscode/compile_commands.json"));

sw.Stop();

Log.Information($"Total: {(sw.ElapsedMilliseconds + before_sw.ElapsedMilliseconds) / 1000.0f}s");
Log.Information($"Prepare Total: {before_sw.ElapsedMilliseconds / 1000.0f}s");
Log.Information($"Execution Total: {sw.ElapsedMilliseconds / 1000.0f}s");
Log.Information($"Compile Commands Total: {CompileCommandsEmitter.Time / 1000.0f}s");
Log.Information($"Compile Total: {CppCompileEmitter.Time / 1000.0f}s");
Log.Information($"Link Total: {CppLinkEmitter.Time / 1000.0f}s");
Log.CloseAndFlush();