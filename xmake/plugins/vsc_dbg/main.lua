import("core.base.option")
import("core.base.task")
import("core.base.global")
import("core.project.config")
import("core.project.project")
import("core.project.depend")
import("core.platform.platform")
import("core.base.json")

function main()
    -- load config
    config.load("build/.gens/analyze.conf")
    -- load targets
    project.load_targets()

    -- get data
    local plat = config.get("plat")
    local arch = config.get("arch")
    local mode = config.get("mode")
    local build_dir = format("${workspaceFolder}/build/%s/%s/%s", plat, arch, mode)

    -- get options
    local opt_targets = option.get("targets")

    -- collect targets
    local targets_info = {}
    do
        function _add_target(target)
            local target_kind = target:kind()
            local enable_proxy_func = target:values("vsc_dbg.proxy_func")
            local proxy_func = enable_proxy_func and target:script("build") or nil
            
            if proxy_func then
                local proxy_results = {}
                proxy_func(target, proxy_results)
                for _, proxy_result in ipairs(proxy_results) do
                    -- check required fields
                    if not proxy_result.name then
                        raise(target:name()..": proxy_result.name is required!")
                    end
                    if not proxy_result.program then
                        raise(target:name()..": proxy_result.program is required!")
                    end

                    -- check proxy_result fields
                    local fields = {
                        name = true, 
                        program = true, 
                        args = true, 
                        envs = true, 
                        pre_cmd = true, 
                        post_cmd = true
                    }
                    for k, v in pairs(proxy_result) do
                        if not fields[k] then
                            raise(target:name()..": proxy_result."..k.." is invalid!")
                        end
                    end
                    
                    -- insert proxy result
                    table.insert(targets_info, {
                        target = target,
                        pre_task_cmd = nil,
                        post_task_cmd = nil,
                        proxy_result = proxy_result,
                    })
                end
            elseif target_kind == "binary"then
                table.insert(targets_info, {
                    target = target,
                    pre_task_cmd = nil,
                    post_task_cmd = nil,
                    proxy_result = nil,
                })
            end
        end

        if opt_targets and #opt_targets > 0 then -- collect targets from command line
            for _, target_name in ipairs(opt_targets) do
                local target = project.target(target_name)
                if target then
                    _add_target(target)
                else
                    cprint("${yellow}target [%s] not found!${clear}", target_name)
                end
            end
        else -- global collect targets
            for _, target in pairs(project.ordertargets()) do
                _add_target(target)
            end
        end
    end

    -- build tasks
    for _, target_info in ipairs(targets_info) do
        local pre_cmd = ""
        local post_cmd = ""

        -- build cmd
        if target_info.proxy_result then
            pre_cmd = target_info.proxy_result.pre_cmd
            post_cmd = target_info.proxy_result.post_cmd
        else
            local config_pre_cmd = target_info.target:values("vsc_dbg.cmd_prev")
            local config_post_cmd = target_info.target:values("vsc_dbg.cmd_post")
            
            -- combine pre cmd
            pre_cmd = "xmake build " .. target_info.target:name()
            if config_pre_cmd then
                for _, cmd in ipairs(config_pre_cmd) do
                    pre_cmd = pre_cmd .. "\n" .. cmd
                end
            end

            -- combine post cmd
            if config_post_cmd then
                for _, cmd in ipairs(config_post_cmd) do
                    post_cmd = post_cmd .. "\n" .. cmd
                end
            end
        end
        
        -- write cmd
        local pre_cmd_file_name = "build/vsc_dbg/pre_" .. target_info.target:name() .. ".bat"
        local post_cmd_file_name = "build/vsc_dbg/post_" .. target_info.target:name() .. ".bat"
        if pre_cmd and #pre_cmd > 0 then
            os.rm(pre_cmd_file_name)
            io.writefile(pre_cmd_file_name, pre_cmd)
            target_info.pre_task_cmd = pre_cmd_file_name
        end
        if post_cmd and #post_cmd > 0 then
            os.rm(post_cmd_file_name)
            io.writefile(post_cmd_file_name, post_cmd)
            target_info.post_task_cmd = post_cmd_file_name
        end
    end
    
    -- gen config contents
    local launches = {}
    local tasks = {}
    do
        function _launch(target_info)
            if target_info.proxy_result then
                local proxy_result = target_info.proxy_result

                -- get launch args
                local args = proxy_result.args and proxy_result.args or json.mark_as_array({})

                -- get launch envs
                local envs = json.mark_as_array({})
                for k, v in pairs(proxy_result.envs) do
                    table.insert(envs, {
                        name = k,
                        value = v
                    })
                end

                table.insert(launches, {
                    name = "▶️" .. proxy_result.name,
                    type = "cppvsdbg",
                    request = "launch",
                    program = proxy_result.program,
                    args = args,
                    stopAtEntry = false,
                    cwd = build_dir,
                    environment = envs,
                    console = "integratedTerminal",
                    preLaunchTask = target_info.pre_task_cmd and "pre " .. target:name() or nil,
                    postLaunchTask = target_info.post_task_cmd and "post " .. target:name() or nil,
                })
            else
                local target = target_info.target
                local config_args = target:values("vsc_dbg.args")
                local config_envs = target:values("vsc_dbg.env")
                
                -- get launch args
                local args = config_args and config_args or json.mark_as_array({})

                -- get launch envs
                local envs = json.mark_as_array({})
                for _, k in ipairs(config_envs) do
                    local v = target:values("vsc_dbg.env." .. k)
                    table.insert(envs, {
                        name = k,
                        value = v
                    })
                end

                -- append
                table.insert(launches, {
                    name = "▶️" .. target:name(),
                    type = "cppvsdbg",
                    request = "launch",
                    program = format("%s/%s.exe", build_dir, target:name()),
                    args = args,
                    stopAtEntry = false,
                    cwd = build_dir,
                    environment = envs,
                    console = "integratedTerminal",
                    preLaunchTask = target_info.pre_task_cmd and "pre " .. target:name() or nil,
                    postLaunchTask = target_info.post_task_cmd and "post " .. target:name() or nil,
                })
            end
        end
        function _tasks(target_info)
            if target_info.pre_task_cmd then
                table.insert(tasks, {
                    label = "pre " .. target_info.target:name(),
                    type = "shell",
                    command = target_info.pre_task_cmd,
                    options = {
                        cwd = "${workspaceFolder}",
                    },
                    group = {
                        kind = "build",
                        isDefault = false
                    },
                    presentation = {
                        echo = true,
                        reveal = "always",
                        focus = false,
                        panel = "new",
                        showReuseMessage = false,
                        close = true,
                    }
                })
            end
            if target_info.post_task_cmd then
                table.insert(tasks, {
                    label = "post " .. target_info.target:name(),
                    type = "shell",
                    command = target_info.post_task_cmd,
                    options = {
                        cwd = "${workspaceFolder}",
                    },
                    group = {
                        kind = "build",
                        isDefault = false
                    },
                    presentation = {
                        echo = true,
                        reveal = "always",
                        focus = false,
                        panel = "new",
                        showReuseMessage = false,
                        close = true,
                    }
                })
            end
        end

        -- generate debug configurations for exec targets
        for _, target_info in ipairs(targets_info) do
            _launch(target_info)
            _tasks(target_info)
        end

        -- attach launch needn't bound with target
        table.insert(launches, {
            name = "🔍Attach",
            type = "cppvsdbg",
            request = "attach",
            processId = "${command:pickProcess}",
            -- program = format("%s/%s.dll", build_dir, target:name()),
            -- args = json.mark_as_array({}),
            -- stopAtEntry = false,
            -- cwd = build_dir,
            -- environment = json.mark_as_array({}),
            -- console = "externalTerminal",
            -- preLaunchTask = "build "..target:name(),
        })
    end

    -- save debug configurations
    json.savefile(".vscode/launch.json", {
        version = "0.2.0",
        configurations = launches
    })
    json.savefile(".vscode/tasks.json", {
        version = "2.0.0",
        tasks = tasks
    })
end
