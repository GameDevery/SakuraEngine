using SB;
using SB.Core;
using Serilog;

[TargetScript]
public static class SkrInput
{
    static SkrInput()
    {
        var SkrInput = Engine
            .Module("SkrInput")
            .EnableUnityBuild()
            .Depend(Visibility.Public, "SkrRT")
            .IncludeDirs(Visibility.Public, "include")
            .AddCppFiles("src/*.cpp", "src/common/*.cpp", "src/sdl2/*.cpp");

        if (BuildSystem.TargetOS == OSPlatform.Windows)
        {
            SkrInput
                .Defines(Visibility.Private, "SKR_INPUT_USE_GAME_INPUT")
                .AddCppFiles("src/game_input/**.cpp");
        }
    }
}