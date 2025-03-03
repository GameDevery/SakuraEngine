--[[
target("static0")
    set_kind("static")
    add_rules("sakura.dyn_module", {api = "STATIC0"})
    public_dependency("SkrRT")
    add_files("test/static0.cpp")

target("dynamic0")
    set_kind("shared")
    add_rules("sakura.dyn_module", {api = "DYNAMIC0"})
    public_dependency("SkrRT")
    add_files("test/dynamic0.cpp")

target("dynamic1")
    set_kind("shared")
    add_rules("sakura.dyn_module", {api = "DYNAMIC1"})
    public_dependency("SkrRT")
    add_files("test/dynamic1.cpp")

target("dynamic2")
    set_kind("shared")
    add_rules("sakura.dyn_module", {api = "DYNAMIC2"})
    public_dependency("SkrRT")
    add_files("test/dynamic2.cpp")

target("dynamic3")
    set_kind("shared")
    add_rules("sakura.dyn_module", {api = "DYNAMIC3"})
    public_dependency("SkrRT")
    add_files("test/dynamic3.cpp")

target("ModuleTest")
    set_kind("binary")
    add_rules("sakura.dyn_module", {api = "MODULE_TEST"})
    public_dependency("SkrRT")
    public_dependency("static0")
    public_dependency("dynamic0")
    public_dependency("dynamic1")
    public_dependency("dynamic2")
    public_dependency("dynamic3")
    -- actually there is no need to strongly link these dynamic modules
    add_deps("SkrTestFramework", {public = false})
    add_files("test/main.cpp")
]]--