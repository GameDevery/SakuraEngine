add_requires("luau", { configs = { extern_c = true }})

shared_module("SkrLua", "SKR_LUA")
    public_dependency("SkrRT")
    public_dependency("SkrImGui")
    add_packages("luau", {public = true, inherit = false})
    add_includedirs("include", {public=true})
    add_files("src/**.cpp")
    skr_unity_build()
