includes("modules/anim_debug_runtime") -- runtime module
executable_module("AnimDebug", "ANIM_DEBUG")
    set_group("04.examples/animation")
    set_exceptions("no-cxx")
    public_dependency("AnimDebugRuntime")
    skr_unity_build()
    add_deps("AppSampleCommon")
    add_files("src/**.cpp")

    -- shaders
    add_rules("utils.dxc", {
        spv_outdir = "/../resources/shaders/AnimDebug",
        dxil_outdir = "/../resources/shaders/AnimDebug"})
    add_files("shaders/**.hlsl")
