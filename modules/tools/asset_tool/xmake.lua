add_requires("nativefiledialog")

shared_module("SkrAssetTool", "SKR_ASSET_TOOL")
    set_group("02.tools")
    public_dependency("SkrToolCore")
    public_dependency("SkrTextureCompiler")
    public_dependency("SkrShaderCompiler")
    public_dependency("SkrGLTFTool")
    public_dependency("SkrImGui")
    -- public_dependency("GameTool")
    public_dependency("SkrAnimTool")
    add_files("src/**.cpp")
    add_includedirs("include", {public=true})
    add_packages("nativefiledialog", {public=false})
    set_exceptions("no-cxx")
    add_rules("c++.unity_build", {batchsize = default_unity_batch})

executable_module("SkrAssetImport", "SKR_ASSET_IMPORT")
    set_group("02.tools")
    public_dependency("SkrAssetTool")
    add_files("main.cpp", "imgui.cpp", "imgui_impl_sdl.cpp")
    set_exceptions("no-cxx")
    add_packages("nativefiledialog", {public=false})
    add_rules("c++.unity_build", {batchsize = default_unity_batch})