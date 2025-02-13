set_xmakever("2.9.2")

-- description scope extensions
includes("xmake/skr_desc_ext.lua")

-- global configs
skr_env_set("engine_dir", os.scriptdir())
default_unity_batch = 16

-- setup xmake extensions
add_moduledirs("xmake/modules")
add_plugindirs("xmake/plugins")
add_repositories("skr-xrepo xrepo", {rootdir = os.scriptdir()})
includes("xmake/options.lua")
includes("xmake/compile_flags.lua")
includes("xmake/rules.lua")

-- global configs
skr_global_config()

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
    -- global install tool
    skr_install("download", {
        name = "python-embed-3.13.1",
        install_func = "tool",
        plat = {"windows"},
        after_install = function()
            import("skr.utils")
            local python = utils.find_python()
            os.execv(python, {"-m", "pip", "install", "mako"})
            os.execv(python, {"-m", "pip", "install", "autopep8"})
        end
    })
    skr_install("custom", {
        name = "system python",
        plat = {"macosx", "linux"},
        func = function ()
            import("skr.utils")
            local python = utils.find_python()
            if python then
                os.execv(python, {"-m", "pip", "install", "mako"})
                os.execv(python, {"-m", "pip", "install", "autopep8"})
            else
                raise("python not found, please install python3 first")
            end
        end
    })
    skr_install("download", {
        name = "meta-v1.0.1-llvm_18.1.6",
        install_func = "tool",
    })
    skr_install("download", {
        name = "tracy-gui-0.10.1a",
        install_func = "tool",
        arch = {"x64", "x86", "x86_64"} -- TODO. support arm64
    })
skr_global_target_end()

-- modules
includes("thirdparty")
includes("modules")
skr_includes_with_cull("samples", function ()
    includes("samples")
end)
skr_includes_with_cull("tests", function ()
    includes("tests")
end)

-- commit env
if not skr_env_is_committed() then
    skr_env_commit()
end