using SB;
using SB.Core;
using Serilog;

[TargetScript(TargetCategory.Tool)]
public static class LLVMTools
{
    static LLVMTools()
    {
        bool UsePrecompiledCompiler = false;
        if (UsePrecompiledCompiler)
            return;

        Engine.AddDoctor<LLVMDoctor>();

        Engine.Target("MetaCompiler")
            .TargetType(TargetType.Executable)
            .AddCppFiles("meta/src/**.cpp");
    }
}

public class LLVMDoctor : IDoctor
{
    public bool Check()
    {
        Install.SDK("")
        return true;
    }

    public bool Fix()
    {
        return true;
    }
}