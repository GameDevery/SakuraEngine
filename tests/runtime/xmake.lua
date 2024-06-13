test_target("MathTest")
    set_group("05.tests/runtime")
    public_dependency("SkrRT", engine_version)
    add_files("math/math.cpp")

test_target("GoapTest")
    set_group("05.tests/runtime")
    public_dependency("SkrRT", engine_version)
    add_files("goap/test_goap.cpp")

test_target("GraphTest")
    set_group("05.tests/runtime")
    public_dependency("SkrRT", engine_version)
    public_dependency("SkrRenderGraph", engine_version)
    add_files("graph/graph.cpp")

test_target("VFSTest")
    set_group("05.tests/runtime")
    public_dependency("SkrRT", engine_version)
    add_files("vfs/main.cpp")

test_target("IOServiceTest")
    set_group("05.tests/runtime")
    public_dependency("SkrRT", engine_version)
    add_files("io_service/main.cpp")

test_target("SceneTest")
    set_group("05.tests/runtime")
    public_dependency("SkrScene", engine_version)
    add_rules("c++.unity_build", {batchsize = default_unity_batch})
    add_files("scene/main.cpp")

test_target("ECSTest-CStyle")
    set_group("05.tests/runtime")
    public_dependency("SkrRT", engine_version)
    add_files("ecs/c_style.cpp")

test_target("ECSTest-CPPStyle")
    set_group("05.tests/runtime")
    public_dependency("SkrRT", engine_version)
    add_files("ecs/cpp_style.cpp")

test_target("MDBTest")
    set_group("05.tests/runtime")
    public_dependency("SkrRT", engine_version)
    public_dependency("SkrLightningStorage", engine_version)
    add_files("mdb/main.cpp")

--------------------------------------------------------------------------------------

codegen_component("RTTRTest", { api = "RTTR_TEST", rootdir = "rttr" })
    add_files("rttr/**.hpp")

executable_module("RTTRTest", "RTTR_TEST", engine_version)
    set_group("05.tests/runtime")
    set_kind("binary")
    public_dependency("SkrRT", engine_version)
    add_deps("SkrTestFramework", {public = false})
    add_files("rttr/**.cpp")

--------------------------------------------------------------------------------------

codegen_component("TraitTest", { api = "TRAIT_TEST", rootdir = "trait" })
    add_files("trait/**.hpp")

executable_module("TraitTest", "TRAIT_TEST", engine_version, {exception = true})
    set_group("05.tests/runtime")
    set_exceptions("no-cxx")
    public_dependency("SkrRT", engine_version)
    add_deps("SkrTestFramework", {public = false})
    add_files("trait/trait_test.cpp")

--------------------------------------------------------------------------------------

if (false) then
    executable_module("V8Test", "V8_TEST", engine_version)
        set_group("05.tests/runtime")
        public_dependency("SkrV8", engine_version)
        -- add_deps("SkrTestFramework", {public = false})
        add_files("v8/hello_fucking_google.cpp")
end

-- includes("module/xmake.lua")
-- includes("wasm/xmake.lua")