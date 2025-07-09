using SB;
using SB.Core;
using Serilog;

[TargetScript]
public static class SkrSystem
{
    static SkrSystem()
    {
        var SkrSystem = Engine.Module("SkrSystem", "SKR_SYSTEM")
            .EnableUnityBuild()
            .Depend(Visibility.Public, "SkrRT")
            .Depend(Visibility.Private, "SDL3")
            .IncludeDirs(Visibility.Public, "include")
            .AddCppFiles(
                "src/advanced_input/*.cpp",
                "src/advanced_input/common/**.cpp",
                "src/advanced_input/sdl3/**.cpp"
            )
            .AddCppFiles("src/system/**.cpp");

        if (BuildSystem.TargetOS == OSPlatform.Windows)
        {
            SkrSystem
                .Defines(Visibility.Private, "SKR_INPUT_USE_GAME_INPUT")
                .AddCppFiles("src/advanced_input/game_input/**.cpp")
                .AddCppFiles("src/system/win32/**.cpp");
        }
    }
}