using SB;
using SB.Core;

[TargetScript]
public static class SkrImGui
{
    static SkrImGui()
    {
        Engine.AddDoctor<ImGuiDoctor>();

        Engine.Module("SkrImGui", "SKR_IMGUI")
            .Depend(Visibility.Private, "SDL3")
            .Depend(Visibility.Public, "SkrInput", "SkrRenderGraph")
            .IncludeDirs(Visibility.Public, "include")
            .AddCppFiles("src/**.cpp")
            .AddCppSLFiles("shaders/*.cppsl")
            .IncludeDirs(Visibility.Public, "imgui/include")
            .IncludeDirs(Visibility.Private, "imgui/src")
            .AddCppFiles("imgui/src/**.cpp")
            .Defines(Visibility.Public, "IMGUI_DEFINE_MATH_OPERATORS");
    }
}

public class ImGuiDoctor : IDoctor
{
    public bool Check()
    {
        var Font = Install.File("SourceSansPro-Regular.ttf", "resources/font");
        Font.Wait();
        return Font.Result != null;
    }
    public bool Fix()
    {
        return true;
    }
}