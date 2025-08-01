test_target("MathTest")
    set_group("05.tests/runtime")
    public_dependency("SkrRT")
    add_files("math/math.cpp")

test_target("GoapTest")
    set_group("05.tests/runtime")
    public_dependency("SkrRT")
    add_files("goap/test_goap.cpp")

test_target("GraphTest")
    set_group("05.tests/runtime")
    public_dependency("SkrRT")
    public_dependency("SkrRenderGraph")
    add_files("graph/graph.cpp")

test_target("VFSTest")
    set_group("05.tests/runtime")
    public_dependency("SkrRT")
    add_files("vfs/main.cpp")

test_target("IOServiceTest")
    set_group("05.tests/runtime")
    public_dependency("SkrRT")
    add_files("io_service/main.cpp")

test_target("SceneTest")
    set_group("05.tests/runtime")
    public_dependency("SkrScene")
    skr_unity_build()
    add_files("scene/main.cpp")

test_target("ECSTest_CStyle")
    set_group("05.tests/runtime")
    public_dependency("SkrRT")
    add_files("ecs/c_style/*.cpp")

codegen_component("ECSTest_CPPStyle", { api = "ECS_TEST", rootdir = "ecs/cpp_style" })
    add_files("ecs/cpp_style/**.hpp")

executable_module("ECSTest_CPPStyle", "ECS_TEST")
    set_group("05.tests/runtime")
    public_dependency("SkrRT")
    add_deps("SkrTestFramework", {public = false})
    add_files("ecs/cpp_style/*.cpp")


--------------------------------------------------------------------------------------
executable_module("RCTest", "RC_TEST")
    set_group("05.tests/runtime")
    set_kind("binary")
    public_dependency("SkrRT")
    add_deps("SkrTestFramework", {public = false})
    add_files("rc/**.cpp")

--------------------------------------------------------------------------------------

codegen_component("RTTRTest", { api = "RTTR_TEST", rootdir = "rttr" })
    add_files("rttr/**.hpp")

executable_module("RTTRTest", "RTTR_TEST")
    set_group("05.tests/runtime")
    set_kind("binary")
    public_dependency("SkrRT")
    add_deps("SkrTestFramework", {public = false})
    add_files("rttr/**.cpp")

--------------------------------------------------------------------------------------

codegen_component("ProxyTest", { api = "PROXY_TEST", rootdir = "proxy" })
    add_files("proxy/**.hpp")

executable_module("ProxyTest", "PROXY_TEST", {exception = true})
    set_group("05.tests/runtime")
    set_exceptions("no-cxx")
    public_dependency("SkrRT")
    add_deps("SkrTestFramework", {public = false})
    add_files("proxy/**.cpp")

--------------------------------------------------------------------------------------

skr_includes_with_cull("v8", function ()
    codegen_component("V8Test", { api = "V8_TEST", rootdir = "v8" })
        add_files("v8/**.hpp")
    executable_module("V8Test", "V8_TEST")
        set_group("05.tests/runtime")
        public_dependency("SkrV8")
        add_deps("SkrTestFramework", {public = false})
        add_files("v8/**.cpp")

    codegen_component("V8TestNew", { api = "V8_TEST_NEW", rootdir = "v8_new" })
        add_files("v8_new/**.hpp")
    executable_module("V8TestNew", "V8_TEST_NEW")
        set_group("05.tests/runtime")
        public_dependency("SkrV8New")
        add_deps("SkrTestFramework", {public = false})
        add_files("v8_new/**.cpp")
end)
-- includes("module/xmake.lua")