using SB;
using SB.Core;
using Serilog;

[TargetScript]
public static class Game
{
    static Game()
    {
        Engine.Program("Game", "GAME")
            .Depend(Visibility.Private, "GameRuntime")
            .Depend(Visibility.Private, "AppSampleCommon")
            .IncludeDirs(Visibility.Private, "include")
            .AddCppFiles("src/*.cpp")
            .AddHLSLFiles("shaders/*.hlsl")
            .DXCOutputDirectory("resources/shaders/Game")
            .CopyFilesWithRoot(
                RootDir: "resources",
                Destination: "resources/Game",
                "resources/**.gltf",
                "resources/**.bin",
                "resources/**.meta"
            );
    }
}