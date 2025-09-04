using SB;
using SB.Core;
using Serilog;

[TargetScript]
public static class OGUI_Sandbox
{
    static OGUI_Sandbox()
    {
        Engine.Program("OGUI_Sandbox", "OGUI_SANDBOX")
            .Require("SDL3", new PackageConfig { Version = new Version(1, 0, 0) })
            .Depend(Visibility.Public, "SDL3@SDL3")
            .Depend(Visibility.Public, "SkrInputSystem", "SkrImGui", "SkrGui", "SkrGuiRenderer")
            .IncludeDirs(Visibility.Private, "./../common")
            .AddCppFiles("src/*.cpp")
            .EnableCodegen("src")
            .AddMetaHeaders("src/**.hpp");
    }
}
