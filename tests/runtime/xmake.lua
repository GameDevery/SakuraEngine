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
    add_rules("c++.unity_build", {batchsize = default_unity_batch})
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

if (false) then
    executable_module("V8Test", "V8_TEST")
        set_group("05.tests/runtime")
        public_dependency("SkrV8")
        -- add_deps("SkrTestFramework", {public = false})
        add_files("v8/hello_fucking_google.cpp")
end

-- includes("module/xmake.lua")