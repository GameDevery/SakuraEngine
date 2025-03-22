using SB;
using SB.Core;
using Serilog;

[TargetScript]
public static class SkrImGui
{
    static SkrImGui()
    {
        Engine
            .Module("SkrImGui", "SKR_IMGUI")
            
            .Require("imgui", new ImGuiPackageConfig { Version = new Version(1, 89, 0), ImportDynamicAPIFromEngine = !Engine.ShippingOneArchive })
            .Depend(Visibility.Public, "imgui@imgui")

            .Depend(Visibility.Public, "SkrInput", "SkrRenderGraph")
            .Defines(Visibility.Private, "\"IMGUI_IMPORT=extern SKR_IMGUI_API\"")

            .IncludeDirs(Visibility.Public, Path.Combine(SourceLocation.Directory(), "include"))
            .AddFiles("src/build.*.cpp");
    }
}