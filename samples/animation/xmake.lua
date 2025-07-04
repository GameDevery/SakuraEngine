includes("debug_runtime")

target("SkrAnimTemp") -- template sample for dev, remove later
    set_group("04.examples/animation")
    set_kind("binary")
    set_exceptions("no-cxx")
    skr_unity_build()
    public_dependency("AnimDebugRuntime")
    add_deps("AppSampleCommon")
    add_files("anim_temp.cpp")
target_end()
