executable_module("SkrResourceCompiler", "SKR_RESOURCE_COMPILER")
    set_group("02.tools")
    set_exceptions("no-cxx")
    public_dependency("SkrToolCore")
    public_dependency("SkrTextureCompiler")
    public_dependency("SkrShaderCompiler")
    public_dependency("SkrGLTFTool")
    -- public_dependency("GameTool")
    public_dependency("SkrAnimTool")
    add_rules("c++.unity_build", {batchsize = default_unity_batch})
    add_files("**.cpp")