using SB;
using SB.Core;
using Serilog;
using System.Diagnostics;

var Toolchain = Utilities.Bootstrap(SourceLocation.Directory());
var ToolchainInitializeTask = Toolchain.Initialize();

Stopwatch sw = new();
sw.Start();
var DBInitializeTask = DependContext.Initialize();

var CompileCommandsEmitter = new CompileCommandsEmitter(Toolchain);
BuildSystem.AddTaskEmitter("Cpp.CompileCommands", CompileCommandsEmitter);

BuildSystem.AddTaskEmitter("ModuleMeta", new ModuleMetaEmitter());

BuildSystem.AddTaskEmitter("Codgen.Meta", new CodegenMetaEmitter(Toolchain, Path.Combine(SourceLocation.Directory(), "build/.skr/tool/windows")));

BuildSystem.AddTaskEmitter("Codgen.Codegen", new CodegenRenderEmitter(Toolchain))
    .AddDependency("Codgen.Meta", DependencyModel.ExternalTarget)
    .AddDependency("Codgen.Meta", DependencyModel.PerTarget);

BuildSystem.AddTaskEmitter("Cpp.Compile", new CppCompileEmitter(Toolchain))
    .AddDependency("ModuleMeta", DependencyModel.PerTarget)
    .AddDependency("Codgen.Codegen", DependencyModel.ExternalTarget)
    .AddDependency("Codgen.Codegen", DependencyModel.PerTarget);

BuildSystem.AddTaskEmitter("Cpp.Link", new CppLinkEmitter(Toolchain))
    .AddDependency("Cpp.Link", DependencyModel.ExternalTarget)
    .AddDependency("Cpp.Compile", DependencyModel.PerTarget);

Engine.Module("TestTarget")
    .TargetType(TargetType.Static)

    .CppVersion("20")

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


await ToolchainInitializeTask;
await DBInitializeTask;
BuildSystem.RunBuild();

CompileCommandsEmitter.WriteToFile(Path.Combine(SourceLocation.Directory(), ".vscode/compile_commands.json"));

sw.Stop();

Log.Information($"Total: {sw.ElapsedMilliseconds / 1000.0f}s");
Log.Information($"Compile Commands Total: {CompileCommandsEmitter.Time / 1000.0f}s");
Log.Information($"Compile Total: {CppCompileEmitter.Time / 1000.0f}s");
Log.Information($"Link Total: {CppLinkEmitter.Time / 1000.0f}s");
Log.CloseAndFlush();