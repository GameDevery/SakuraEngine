set_xmakever("2.9.2")
set_project("SakuraEngine")

default_unity_batch = 16

-- global options
set_warnings("all")
set_policy("build.ccache", false)
set_policy("build.warning", true)
set_policy("check.auto_ignore_flags", false)

-- setup xmake extensions
add_moduledirs("xmake/modules")
add_plugindirs("xmake/plugins")
add_repositories("skr-xrepo xrepo", {rootdir = os.scriptdir()})

-- cxx options
set_languages(get_config("cxx_version"), get_config("c_version"))
add_rules("mode.debug", "mode.release", "mode.releasedbg", "mode.asan")

-- xmake extensions
includes("xmake/options.lua")
includes("xmake/compile_flags.lua")
includes("xmake/rules.lua")

-- add global rules to all targets
add_rules("DisableTargets")

-- load project configs
if os.exists("project.lua") then
    includes("project.lua")
else
    includes("./xmake/project.default.lua")
end

-- global install tools
skr_global_target()
    skr_install("download", {
        name = "meta-v1.0.1-llvm_17.0.6",
        install_func = "tool",
    })
    skr_install("download", {
        name = "tracy-gui-0.10.1a",
        install_func = "tool",
    })
skr_global_target_end()

-- cxx defines
if (is_os("windows")) then 
    add_defines("UNICODE", "NOMINMAX", "_WINDOWS")
    add_defines("_GAMING_DESKTOP")
    add_defines("_CRT_SECURE_NO_WARNINGS")
    add_defines("_ENABLE_EXTENDED_ALIGNED_STORAGE")
    if (is_mode("release")) then
        set_runtimes("MD")
    elseif (is_mode("asan")) then
        add_defines("_DISABLE_VECTOR_ANNOTATION")
    else
        set_runtimes("MDd")
    end
end

-- modules
includes("xmake/thirdparty.lua")
includes("modules/xmake.lua")
includes("samples/xmake.lua")
includes("editors/xmake.lua")
includes("tests/xmake.lua")