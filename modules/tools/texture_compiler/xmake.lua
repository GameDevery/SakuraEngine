target("ISPCTextureCompressor")
    set_group("02.tools")
    set_policy("build.across_targets_in_parallel", false)
    set_kind("static")
    add_packages("ispc")
    add_rules("utils.ispc")
    add_files("src/**.ispc")

codegen_component("SkrTextureCompiler", { api = "SKR_TEXTURE_COMPILER", rootdir = "include/SkrTextureCompiler" })
    add_files("include/**.hpp")

shared_module("SkrTextureCompiler", "SKR_TEXTURE_COMPILER", engine_version)
    set_group("02.tools")
    add_deps("ISPCTextureCompressor")
    public_dependency("SkrRenderer", engine_version)
    public_dependency("SkrImageCoder", engine_version)
    public_dependency("SkrToolCore", engine_version)
    add_rules("c++.unity_build", {batchsize = default_unity_batch})
    add_includedirs("include", {public=true})
    add_files("src/**.cpp")