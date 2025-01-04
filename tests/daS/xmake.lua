-- simple interpret
target("daSTestInterpret")
    set_kind("binary")
    set_group("05.tests/daS")
    public_dependency("SkrDAScript")
    add_deps("SkrTestFramework", {public = false})
    add_files("daSTestInterpret/**.cpp")

-- AOT (WIP)
--[[
target("daSTestAOT")
    set_kind("binary")
    set_group("05.tests/daS")
    add_rules("daScript", {
        outdir = "./scripts",
        rootdir = os.curdir()
    })
    public_dependency("SkrDAScript")
    add_deps("SkrTestFramework", {public = false})
    add_files("daSTestAOT/**.das", {aot = true})
    add_files("daSTestAOT/**.cpp")
]]--
    
-- Annotation
--[[
target("daSTestAnnotationCompiler")
    set_kind("binary")
    set_group("05.tests/daS")
    public_dependency("SkrDAScript")
    add_files("daSTestAnnotation/annotation_compiler.cpp")
    add_packages("daScript", { public = false })
]]--

target("daSTestAnnotation")
    set_kind("binary")
    set_group("05.tests/daS")
    add_rules("daScript", {
        outdir = "./scripts",
        rootdir = os.curdir()
    })
    public_dependency("SkrDAScript")
    add_deps("SkrTestFramework", {public = false})
    add_files("daSTestAnnotation/**.das")
    add_files("daSTestAnnotation/test_annotation.cpp")

-- Coroutine
target("dasCo")
    set_kind("binary")
    set_group("05.tests/daS")
    add_rules("daScript", {
        outdir = "./scripts",
        rootdir = os.curdir()
    })
    public_dependency("SkrDAScript")
    add_deps("SkrTestFramework", {public = false})
    add_files("dasCo/**.das")
    add_files("dasCo/**.cpp")

-- AOT (WIP)
--[[
target("dasCoAOT")
    set_kind("binary")
    set_group("05.tests/daS")
    add_rules("daScript", {
        outdir = "./scripts",
        rootdir = os.curdir()
    })
    public_dependency("SkrDAScript")
    add_deps("SkrTestFramework", {public = false})
    add_defines("AOT")
    add_files("dasCo/**.das", {aot = true})
    add_files("dasCo/**.cpp")
]]--

-- Stackwalk
target("dasStackwalk")
    set_kind("binary")
    set_group("05.tests/daS")
    add_rules("daScript", {
        outdir = "./scripts",
        rootdir = os.curdir()
    })
    public_dependency("SkrDAScript")
    add_deps("SkrTestFramework", {public = false})
    add_files("dasStackwalk/**.das")
    add_files("dasStackwalk/**.cpp")

--[[
target("dasStackwalkAOT")
    set_kind("binary")
    set_group("05.tests/daS")
    add_rules("daScript", {
        outdir = "./scripts",
        rootdir = os.curdir()
    })
    public_dependency("SkrDAScript")
    add_deps("SkrTestFramework", {public = false})
    add_defines("AOT")
    add_files("dasStackwalk/**.das", {aot = true})
    add_files("dasStackwalk/**.cpp")

-- Hybrid
target("daSTestHybrid")
    set_kind("binary")
    set_group("05.tests/daS")
    add_rules("daScript", {
        outdir = "./scripts",
        rootdir = os.curdir()
    })
    public_dependency("SkrDAScript")
    add_deps("SkrTestFramework", {public = false})
    add_files("daSTestHybrid/aot/**.das", {aot = true})
    add_files("daSTestHybrid/script/**.das", {aot = false})
    add_files("daSTestHybrid/**.cpp")
]]--