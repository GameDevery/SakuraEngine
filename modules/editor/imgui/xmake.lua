shared_module("SkrImGuiNG", "SKR_IMGUI_NG")
    -- misc
    skr_unity_build()

    -- deps
    add_deps("ImGui")
    public_dependency("SkrInput")
    public_dependency("SkrRenderGraph")

    -- includes
    add_includedirs("include", {public=true})
    add_files("src/**.cpp")

    -- for export ImGui API
    add_defines("IMGUI_IMPORT= extern SKR_IMGUI_NG_API", {public=false})

    -- font files
    skr_install_rule()
    skr_install("download", {
        name = "SourceSansPro-Regular.ttf",
        install_func = "file",
        out_dir = "resources/font"
    })
