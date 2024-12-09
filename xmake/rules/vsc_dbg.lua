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

-- -- @command: command string
-- function skr_dbg_cmd_pre(command)
--     add_values("vsc_dbg.cmd_prev", command)
-- end

-- -- @command: command string
-- function skr_dbg_cmd_post(command)
--     add_values("vsc_dbg.cmd_post", command)
-- end
