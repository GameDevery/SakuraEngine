if(has_config("shipping_one_archive")) then
    add_requires("imgui >=1.89.0-skr", { configs = { runtime_shared = false } })
else
    add_requires("imgui >=1.89.0-skr", { configs = { runtime_shared = true } })
end

shared_module("SkrImGui", "SKR_IMGUI")
    public_dependency("SkrInput")
    public_dependency("SkrRenderGraph")
    add_packages("imgui", {public=true})
    skr_unity_build()
    add_includedirs("include", {public=true})
    add_defines("IMGUI_IMPORT= extern SKR_IMGUI_API", {public=false})
    add_files("src/build.*.cpp")
    -- add render graph shaders
    add_rules("utils.dxc", {
        spv_outdir = "/../resources/shaders", 
        dxil_outdir = "/../resources/shaders"})
    add_files("shaders/*.hlsl")

    skr_install_rule()
    skr_install("download", {
        name = "SourceSansPro-Regular.ttf",
        install_func = "file",
        out_dir = "resources/font"
    })