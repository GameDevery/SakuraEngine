using SB;
using SB.Core;
using Serilog;
using System.Diagnostics;

Stopwatch before_sw = new();
before_sw.Start();

var Toolchain = Engine.Bootstrap(SourceLocation.Directory());

Engine.AddTaskEmitter("DXC.Compile", new DXCEmitter());

Engine.AddTaskEmitter("ModuleMeta", new ModuleMetaEmitter());

Engine.AddTaskEmitter("Codgen.Meta", new CodegenMetaEmitter(Toolchain));

Engine.AddTaskEmitter("Codgen.Codegen", new CodegenRenderEmitter(Toolchain))
    .AddDependency("Codgen.Meta", DependencyModel.ExternalTarget)
    .AddDependency("Codgen.Meta", DependencyModel.PerTarget);

Engine.AddTaskEmitter("Cpp.PCH", new PCHEmitter(Toolchain))
    .AddDependency("Codgen.Codegen", DependencyModel.ExternalTarget)
    .AddDependency("Codgen.Codegen", DependencyModel.PerTarget);

Engine.AddTaskEmitter("Cpp.UnityBuild", new UnityBuildEmitter())
    .AddDependency("ModuleMeta", DependencyModel.PerTarget)
    .AddDependency("Codgen.Codegen", DependencyModel.PerTarget);

Engine.AddTaskEmitter("Cpp.Compile", new CppCompileEmitter(Toolchain))
    .AddDependency("ModuleMeta", DependencyModel.PerTarget)
    .AddDependency("Cpp.UnityBuild", DependencyModel.PerTarget)
    .AddDependency("Cpp.PCH", DependencyModel.PerTarget)
    .AddDependency("Cpp.PCH", DependencyModel.ExternalTarget)
    .AddDependency("Codgen.Codegen", DependencyModel.ExternalTarget)
    .AddDependency("Codgen.Codegen", DependencyModel.PerTarget);

var CompileCommandsEmitter = new CompileCommandsEmitter(Toolchain);
Engine.AddTaskEmitter("Cpp.CompileCommands", CompileCommandsEmitter)
    .AddDependency("ModuleMeta", DependencyModel.PerTarget)
    .AddDependency("Cpp.UnityBuild", DependencyModel.PerTarget)
    .AddDependency("Cpp.PCH", DependencyModel.PerTarget)
    .AddDependency("Cpp.PCH", DependencyModel.ExternalTarget)
    .AddDependency("Codgen.Codegen", DependencyModel.ExternalTarget)
    .AddDependency("Codgen.Codegen", DependencyModel.PerTarget);

Engine.AddTaskEmitter("Cpp.Link", new CppLinkEmitter(Toolchain))
    .AddDependency("Cpp.Link", DependencyModel.ExternalTarget)
    .AddDependency("Cpp.Compile", DependencyModel.PerTarget);

Engine.SetTagsUnderDirectory("modules/core", TargetTags.Engine);
Engine.SetTagsUnderDirectory("modules/render", TargetTags.Render);
Engine.SetTagsUnderDirectory("modules/gui", TargetTags.GUI);
Engine.SetTagsUnderDirectory("modules/dcc", TargetTags.DCC);
Engine.SetTagsUnderDirectory("modules/devtime", TargetTags.Tool);
Engine.SetTagsUnderDirectory("modules/tools", TargetTags.Tool);
Engine.SetTagsUnderDirectory("modules/experimental", TargetTags.Experimental);

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