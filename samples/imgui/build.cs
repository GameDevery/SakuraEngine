using SB;
using SB.Core;
using Serilog;

[TargetScript]
public static class ImGuiSample {
    static ImGuiSample()
    {
        Engine.Program("ImGuiSample", "IMGUI_SAMPLE")
            .Depend(Visibility.Private, "SkrImGui")
            .AddCppFiles("src/imgui_sample.cpp");
    }
}