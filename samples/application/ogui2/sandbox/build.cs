using SB;
using SB.Core;
using Serilog;

[TargetScript]
public static class OGUI_Sandbox
{
    static OGUI_Sandbox()
    {
        Engine.Program("OGUI_Sandbox", "OGUI_SANDBOX")
            .Depend(Visibility.Public, "SkrInputSystem", "SkrImGui", "SkrGui", "SkrGuiRenderer")
            .IncludeDirs(Visibility.Private, "./../common")
            .AddCppFiles("src/*.cpp")
            .EnableCodegen("src")
            .AddMetaHeaders("src/**.hpp");
    }
}
