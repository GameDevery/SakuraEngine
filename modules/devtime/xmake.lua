 shared_module("SkrDevCore", "SKR_DEVCORE", engine_version)
    set_group("06.devs")
    public_dependency("SkrImGui", engine_version)
    add_rules("c++.unity_build", {batchsize = default_unity_batch})
    add_includedirs("include", {public=true})
    add_files("src/**.cpp")