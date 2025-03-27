add_requires("meshoptimizer >=0.1.0-skr")

codegen_component("SkrMeshCore", { api = "MESH_CORE", rootdir = "include/SkrMeshCore" })
    add_files("include/**.hpp")

shared_module("SkrMeshCore", "MESH_CORE")
    set_group("02.tools")
    public_dependency("SkrToolCore")
    public_dependency("SkrRenderer")
    add_includedirs("include", {public=true})
    add_files("src/**.cpp")
    skr_unity_build()
    -- meshoptimizer
    add_packages("meshoptimizer", {public=true})