shared_module("SkrV8", "SKR_V8")
    -- dependencies
    public_dependency("SkrRT")
    add_deps("v8", {public = true})

    -- add source files
    add_includedirs("include", {public = true})
    add_files("src/**.cpp")