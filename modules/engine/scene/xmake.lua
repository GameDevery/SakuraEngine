codegen_component("SkrScene", { api = "SKR_SCENE", rootdir = "include/SkrScene" })
    add_files("include/**.h")


shared_module("SkrScene", "SKR_SCENE")
    public_dependency("SkrRT")
    -- public_dependency("SkrLua") -- FIXME. lua support
    skr_unity_build()
    add_includedirs("include", {public=true})
    add_files("src/*.cpp")