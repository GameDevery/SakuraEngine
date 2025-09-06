
using SB;
using SB.Core;

// load all assemblies for commands
AppDomain.CurrentDomain.Load("SB.Core");
AppDomain.CurrentDomain.Load("SB.Engine");

// setup engine directory
Engine.SetEngineDirectory(SourceLocation.Directory());
Engine.SetProjectRoot(SourceLocation.Directory());

return SB.Cli.ReflCommand.InvokeDefaultCommandFromDomain(AppDomain.CurrentDomain, args);
