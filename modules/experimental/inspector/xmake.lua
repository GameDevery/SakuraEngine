 shared_module("SkrInspector", "SKR_INSPECT")
    public_dependency("SkrDevCore")
    add_rules("c++.unity_build", {batchsize = default_unity_batch})
    add_includedirs("include", {public=true})
    add_files("src/**.cpp")