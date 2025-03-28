target("SkrTestFramework")
    set_kind("static")
    set_group("05.tests/framework")
    add_deps("SkrCore")
    set_exceptions("no-cxx")
    add_includedirs("framework/include", {public = true})
    add_includedirs("framework/include/SkrTestFramework", {public = true})
    add_files("framework/src/framework.cpp")

--[[
shared_pch("SkrTestFramework")
    add_files("framework/include/SkrTestFramework/framework.hpp")
]]--

function test_target(name)
    target(name)
        set_kind("binary")
        add_deps("SkrTestFramework", {public = false})
        set_exceptions("no-cxx")
end

-- includes("daS/xmake.lua")
includes("base/xmake.lua")
includes("core/xmake.lua")
includes("memory/xmake.lua")

includes("cgpu/xmake.lua")
includes("runtime/xmake.lua")
includes("async/xmake.lua")