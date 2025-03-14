using SB;
using SB.Core;
using Serilog;
using System.Diagnostics;

var Toolchain = Utilities.Bootstrap(SourceLocation.Directory());
var ToolchainInitializeTask = Toolchain.Initialize();

Stopwatch sw = new();
sw.Start();
var DBInitializeTask = DependContext.Initialize();

BuildSystem.AddTaskEmitter("Cpp.Compile", new CppCompileEmitter(Toolchain));
BuildSystem.AddTaskEmitter("Cpp.Link", new CppLinkEmitter(Toolchain))
    .AddDependency("Cpp.Link", DependencyModel.ExternalTarget)
    .AddDependency("Cpp.Compile", DependencyModel.PerTarget);


BuildSystem.Target("TestTarget")
    .TargetType(TargetType.Static)
    .CppVersion("20")

    .Require("MeshOptimizer", new PackageConfig { Version = new Version(0, 0, 1) })
    .Depend("MeshOptimizer@MeshOptimizer")

    .Require("zlib", new PackageConfig { Version = new Version(1, 2, 8) })
    .Depend("zlib@zlib")

    .Require("yyjson", new PackageConfig { Version = new Version(0, 9, 0) })
    .Depend("yyjson@yyjson")

    .Require("harfbuzz", new PackageConfig { Version = new Version(7, 1, 0) })
    .Depend("harfbuzz@harfbuzz");


await ToolchainInitializeTask;
await DBInitializeTask;
BuildSystem.RunBuild();

sw.Stop();

Log.Information($"Total: {sw.ElapsedMilliseconds / 1000.0f}s");
Log.Information($"Compile Total: {CppCompileEmitter.Time / 1000.0f}s");
Log.Information($"Link Total: {CppLinkEmitter.Time / 1000.0f}s");
Log.CloseAndFlush();