codegen_component("SkrRenderer", { api = "SKR_RENDERER", rootdir = "include/SkrRenderer" })
    add_files("include/**.h")
    add_files("include/**.hpp")

shared_module("SkrRenderer", "SKR_RENDERER", engine_version)
    public_dependency("SkrScene", engine_version)
    public_dependency("SkrRenderGraph", engine_version)
    public_dependency("SkrImGui", engine_version)
    add_includedirs("include", {public=true})
    add_rules("c++.unity_build", {batchsize = default_unity_batch})
    add_files("src/*.cpp")
    add_files("src/resources/*.cpp", {unity_group = "resources"})