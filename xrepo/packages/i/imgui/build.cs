using SB;
using SB.Core;

public record ImGuiPackageConfig : PackageConfig
{
    public bool ImportDynamicAPIFromEngine = true;
}

[TargetScript]
public static class ImGui
{
    static ImGui()
    {
        BuildSystem.Package("imgui")
            .AddTarget("imgui", (Target @this, PackageConfig Config) =>
            {
                bool ImportDynamicAPIFromEngine = false;
                if (Config.Version != new Version(1, 89, 0))
                    throw new TaskFatalError("imgui version mismatch!", "imgui version mismatch, only v1.89.0 is supported in source.");
                if (Config is ImGuiPackageConfig ImGuiConfig)
                    ImportDynamicAPIFromEngine = ImGuiConfig.ImportDynamicAPIFromEngine;

                @this.CppVersion("17")
                    .TargetType(TargetType.Static)
                    .IncludeDirs(Visibility.Public, "port/imgui/include", "port/cimgui/include")
                    .IncludeDirs(Visibility.Private, "port/imgui")
                    .AddCppFiles(
                        "port/imgui/build.*.cpp",
                        "port/cimgui/src/cimgui.cpp"
                    );
                
                if (!ImportDynamicAPIFromEngine)
                    @this.Defines(Visibility.Private, "RUNTIME_ALL_STATIC");
            });
    }
}