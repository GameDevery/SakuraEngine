using SB;
using Cli = SB.Cli;

namespace SB;

public class CompileCommandsCommand : CommandBase
{
    public CompileCommandsCommand()
    {
        CategoryString = "all";
    }

    public override int OnExecute()
    {
        Engine.AddCompileCommandsEmitter(Toolchain);
        Engine.RunBuild();

        Directory.CreateDirectory(".sb/compile_commands/cpp");
        CompileCommandsEmitter.WriteToFile(".sb/compile_commands/cpp/compile_commands.json");

        Directory.CreateDirectory(".sb/compile_commands/shaders");
        CppSLCompileCommandsEmitter.WriteCompileCommandsToFile(".sb/compile_commands/shaders/compile_commands.json");

        return 0;
    }

    [Cli.RegisterCmd(Name = "compile_commands", Help = "Generate Compile Commands for IDEs", Usage = "SB compile_commands [options]")]
    public static object RegisterCommand() => new CompileCommandsCommand();
}