using SB;
using SB.Core;

[TargetScript]
public static class SkrGui
{
    static SkrGui()
    {
        Engine.Module("SkrGui")
            // .EnableUnityBuild()
            .Require("freetype", new PackageConfig { Version = new Version(1, 2, 8) })
            .Require("icu", new PackageConfig { Version = new Version(72, 1, 0) })
            .Require("harfbuzz", new PackageConfig { Version = new Version(7, 1, 0) })
            .Require("nanovg", new PackageConfig { Version = new Version(0, 1, 0) })
            .Depend(Visibility.Public, "SkrRT")
            .Depend(Visibility.Private, "freetype@freetype", "icu@icu", "harfbuzz@harfbuzz", "nanovg@nanovg")
            .IncludeDirs(Visibility.Public, Path.Combine(SourceLocation.Directory(), "include"))
            .IncludeDirs(Visibility.Private, Path.Combine(SourceLocation.Directory(), "src"))
            .AddMetaHeaders("include/**.hpp")
            .AddCppFiles("src/*.cpp")
            .AddCppFiles("src/math/*.cpp")
            .AddCppFiles("src/framework/**.cpp")//, {unity_group = "framework"})
            .AddCppFiles("src/render_objects/**.cpp")//, {unity_group = "render_objects"})
            .AddCppFiles("src/widgets/**.cpp")//, {unity_group = "widgets"})
            .AddCppFiles("src/dev/**.cpp")//, {unity_group = "dev"})
            .AddCppFiles("src/system/**.cpp")//, {unity_group = "system"})

            .AddCppFiles("src/backend/*.cpp")
            .AddCppFiles("src/backend/paragraph/*.cpp")//, {unity_group  = "text"})
            .AddCppFiles("src/backend/text_server/*.cpp")//, {unity_group  = "text"})
            .AddCppFiles("src/backend/text_server_adv/*.cpp")//, {unity_group  = "text_adv"})

            .UsePrivatePCH("include/**.hpp");
    }
}