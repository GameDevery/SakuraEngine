shared_module("SkrOzz", "SKR_OZZ")
    set_exceptions("no-cxx")
    set_optimize("fastest")
    public_dependency("SkrRT")
    skr_unity_build()
    add_includedirs("ozz", {public=true})
    add_includedirs("ozz_src", {public=false})
    add_files("ozz_src/**.cc")
        
private_pch("SkrOzz")
    add_files("ozz/SkrAnim/ozz/*.h")
    
---------------------------------------------------------------------------------------

codegen_component("SkrAnim", { api = "SKR_ANIM", rootdir = "include/SkrAnim" })
    add_files("include/**.h")
    add_files("include/**.hpp")

shared_module("SkrAnim", "SKR_ANIM")
    public_dependency("SkrOzz")
    public_dependency("SkrRenderer")
    add_includedirs("include", {public=true})
    add_includedirs("ozz", {public=true})
    skr_unity_build()
    add_files("src/**.cpp")

private_pch("SkrAnim")
    add_files("src/pch.hpp")