using SB;
using SB.Core;
using Serilog;
using System.Diagnostics;
using Cli = SB.Cli;

namespace SB;

public class TestCommand : CommandBase
{
    public override int OnExecute()
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
            return -1;
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

        return 0;
    }

    [Cli.RegisterCmd(Name = "test", ShortName = 't', Help = "Run tests", Usage = "SB test [options]\nSB t [options]")]
    public static object RegisterCommand() => new TestCommand();
}
