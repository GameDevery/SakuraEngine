-- @args: string list
function skr_dbg_args(args)
    add_values("vsc_dbg.args", args)
end

-- @envs: table, format (string, string)
function skr_dbg_envs(envs)
    for k, v in pairs(envs) do
        add_values("vsc_dbg.env", k)
        set_values("vsc_dbg.env."..k, v)
    end
end

-- @command: command string
function skr_dbg_cmd_pre(command)
    add_values("vsc_dbg.cmd_prev", command)
end

-- @command: command string
function skr_dbg_cmd_post(command)
    add_values("vsc_dbg.cmd_post", command)
end

-- phony target
-- @build_func: function(target, out_configs)
--  use table.insert to add values to [out_configs], config format
--     cmd_name: str (required)
--     program: str (required)
--     label: str (default use cmd_name)
--     args: str list
--     envs: str: str table
--     pre_cmds: str list
--     post_cmds: str list
--     natvis_files: str list
function skr_dbg_proxy_target(name, build_func)
    target(name)
        set_kind("phony")
        set_values("vsc_dbg.proxy_func", true)
        on_build(build_func)
    target_end()
end

function skr_dbg_natvis_files(...)
    add_values("vsc_dbg.natvis_files", ...)
end