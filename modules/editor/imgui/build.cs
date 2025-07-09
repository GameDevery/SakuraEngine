using SB;
using SB.Core;

[TargetScript]
public static class SkrImGui
{
    static SkrImGui()
    {
        Engine.AddSetup<ImGuiSetup>();

        Engine.Module("SkrImGui", "SKR_IMGUI")
            .Depend(Visibility.Public, "SkrSystem", "SkrRenderGraph")
            .IncludeDirs(Visibility.Public, "include")
            .AddCppFiles("src/**.cpp")
            .AddCppSLFiles("shaders/*.cppsl")
            .IncludeDirs(Visibility.Public, "imgui/include")
            .IncludeDirs(Visibility.Private, "imgui/src")
            .AddCppFiles("imgui/src/**.cpp")
            .Defines(Visibility.Public, "IMGUI_DEFINE_MATH_OPERATORS");
    }
}

public class ImGuiSetup : ISetup
{
    public void Setup()
    {
        var Font = Install.File("SourceSansPro-Regular.ttf", "resources/font");
        Font.Wait();
    }
}