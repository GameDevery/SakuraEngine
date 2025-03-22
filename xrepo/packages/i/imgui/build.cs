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
        BuildSystem
            .Package("imgui")
            .AddTarget("imgui", (Target Target, PackageConfig Config) =>
            {
                bool ImportDynamicAPIFromEngine = false;
                if (Config.Version != new Version(1, 89, 0))
                    throw new TaskFatalError("imgui version mismatch!", "imgui version mismatch, only v1.89.0 is supported in source.");
                if (Config is ImGuiPackageConfig ImGuiConfig)
                    ImportDynamicAPIFromEngine = ImGuiConfig.ImportDynamicAPIFromEngine;

                Target
                    .CppVersion("17")
                    .RuntimeLibrary("MD")
                    .TargetType(TargetType.Static)
                    .IncludeDirs(Visibility.Public, Path.Combine(SourceLocation.Directory(), "port/imgui/include"))
                    .IncludeDirs(Visibility.Public, Path.Combine(SourceLocation.Directory(), "port/cimgui/include"))
                    .IncludeDirs(Visibility.Private, Path.Combine(SourceLocation.Directory(), "port/imgui"))
                    .AddFiles(
                        "port/imgui/build.*.cpp",
                        "port/cimgui/src/cimgui.cpp"
                    );
                
                if (!ImportDynamicAPIFromEngine)
                    Target.Defines(Visibility.Private, "RUNTIME_ALL_STATIC");
            });
    }
}