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
-- add_rules("DisableTargets")

-- load project configs
if not disable_project_config then
    if os.exists("project.lua") then
        includes("project.lua")
    else
        includes("./xmake/project.default.lua")
    end
end

-- global install tools
skr_global_target()
    -- global install tool
    -- skr_install("download", {
    --     name = "python-embed-3.13.1",
    --     install_func = "tool",
    --     plat = {"windows"},
    --     after_install = function()
    --         import("skr.utils")
    --         local python = utils.find_python()
    --         -- os.execv(python, {"-m", "pip", "install", "mako"})
    --         os.execv(python, {"-m", "pip", "install", "autopep8"})
    --     end
    -- })
    -- skr_install("custom", {
    --     name = "system python",
    --     plat = {"macosx", "linux"},
    --     func = function ()
    --         import("skr.utils")
    --         local python = utils.find_python()
    --         if python then
    --             -- os.execv(python, {"-m", "pip", "install", "mako"})
    --             os.execv(python, {"-m", "pip", "install", "autopep8"})
    --         else
    --             raise("python not found, please install python3 first")
    --         end
    --     end
    -- })
    skr_install("download", {
        name = "bun_1.2.5",
        install_func = "tool",
        plat = {"windows"},
        after_install = function()
            import("skr.utils")
            local bun = utils.find_bun()
            -- os.execv(bun, {"install", "--production"}, {curdir = path.join(utils.get_env("engine_dir"), "tools/meta_codegen_ts")})
            os.execv(bun, {"install"}, {curdir = path.join(utils.get_env("engine_dir"), "tools/meta_codegen_ts")})
            os.execv(bun, {"install"}, {curdir = path.join(utils.get_env("engine_dir"), "tools/merge_natvis_ts")})
        end
    })
    skr_install("download", {
        name = "meta_v1.0.3-llvm_19.1.7",
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