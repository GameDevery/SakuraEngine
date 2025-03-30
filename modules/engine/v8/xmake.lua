codegen_component("SkrV8", {api = "SKR_V8", rootdir = "include/SkrV8"})
    add_files("include/**.hpp")

shared_module("SkrV8", "SKR_V8")
    -- dependencies
    public_dependency("SkrRT")
    add_deps("v8", {public = true})
    add_deps("libhv", {public = true});

    -- add source files
    add_includedirs("include", {public = true})
    add_files("src/**.cpp")