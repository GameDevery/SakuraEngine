import("core.base.option")
import("core.base.task")
import("core.base.global")
import("core.project.config")
import("core.project.project")
import("core.project.depend")
import("core.platform.platform")
import("core.base.json")
import("skr.utils")

-- TODO. proxy target 创建的 debug 任务需要 custom cwd
-- TODO. 为 proxy target 提供一个工具来复用现有 target 的参数 (natvis/pre_cmds/post_cmds)
-- TODO. compounds launch

-- programs
local _python = utils.find_python()
local _merge_natvis_script_path = path.join(os.projectdir(), "tools/merge_natvis/merge_natvis.py")

-- tools
function _normalize_cmd_name(cmd_name)
    return cmd_name:gsub(" ", "_")
end

-- load generate data from targets
-- proxy targets: used to generate debug target that not consisted to targets
local _proxy_launch_fields = {
    label = true,
    cmd_name = true,
    program = true,
    args = true,
    envs = true,
    pre_cmds = true,
    post_cmds = true,
    natvis_files = true,
}
function _load_launches_from_proxy_target(target, build_dir)
    local enable_proxy_func = target:values("vsc_dbg.proxy_func")
    local proxy_func = enable_proxy_func and target:script("build") or nil

    if proxy_func then
        -- invoke proxy function
        local proxy_results = {}
        proxy_func(target, proxy_results)
    
        local result = {}
        for _, proxy_result in ipairs(proxy_results) do            
            -- check required fields
            if not proxy_result.cmd_name then
                raise(target:name()..": proxy_result.cmd_name is required!")
            end
            if not proxy_result.program then
                raise(target:name()..": proxy_result.program is required!")
            end

            -- check proxy result fields
            for k, v in pairs(proxy_result) do
                if not _proxy_launch_fields[k] then
                    raise(target:name()..": proxy_result."..k.." is invalid!")
                end
            end

            table.insert(result, {
                label = proxy_result.label and proxy_result.label or proxy_result.cmd_name,
                cmd_name = proxy_result.cmd_name,
                program = proxy_result.program,
                args = proxy_result.args or {},
                envs = proxy_result.envs or {},
                cwd = build_dir,
                pre_cmds = proxy_result.pre_cmds or {},
                post_cmds = proxy_result.post_cmds or {},
                natvis_files = proxy_result.natvis_files or {},
            })
        end
        
        if #result > 0 then
            return result
        else
            return nil
        end
    else
        return nil
    end
end
function _load_launch_from_binary_target(target, build_dir)
    if target:kind() == "binary" then
        local config_args = target:values("vsc_dbg.args")
        local config_envs = target:values("vsc_dbg.env")
        local config_pre_cmds = target:values("vsc_dbg.cmd_prev")
        local config_post_cmds = target:values("vsc_dbg.cmd_post")
        
        -- get launch args
        local args = config_args and config_args or {}

        -- get launch envs
        local envs = {}
        for _, k in ipairs(config_envs) do
            local v = target:values("vsc_dbg.env." .. k)
            envs[k] = v
        end
        
        -- get pre cmd
        local pre_cmds = {"xmake build "..target:name()} -- build target anyways
        if config_pre_cmds then
            for _, cmd in ipairs(config_pre_cmds) do
                table.insert(pre_cmds, cmd)
            end
        end

        -- get post cmds
        local post_cmds = config_post_cmds and config_post_cmds or {}

        -- get natvis files
        local natvis_files = {}
        do
            local function _try_add_natvis_file(target)
                local config_natvis_files = target:values("vsc_dbg.natvis_files")
                if config_natvis_files then
                    for _, natvis_file in ipairs(config_natvis_files) do
                        -- translate to absolute path
                        if not path.is_absolute(natvis_file) then
                            natvis_file = path.absolute(natvis_file, target:scriptdir())
                        end

                        -- extract files
                        local extracted_files = os.files(natvis_file)

                        -- add to table
                        for _, file in ipairs(extracted_files) do
                            table.insert(natvis_files, file)
                        end
                    end
                end
            end

            _try_add_natvis_file(target)
            for _, dep in ipairs(target:orderdeps()) do
                _try_add_natvis_file(dep)
            end
        end
        
        return {
            label = target:name(),
            cmd_name = target:name(),
            program = path.join(build_dir, target:name()..".exe"),
            args = args,
            envs = envs,
            cwd = build_dir,
            pre_cmds = pre_cmds,
            post_cmds = post_cmds,
            natvis_files = natvis_files,
        }
    else
        return nil
    end
end

-- generate tasks
local _task_cmd_dir = "build/.skr/vsc_dbg/"
-- @return: file name
function _generate_cmd_file(cmds, cmd_name)
    if cmds and #cmds > 0 then
        -- combines cmd str
        local cmd_str = ""
        local check_result = "IF %ERRORLEVEL% NEQ 0 (exit %ERRORLEVEL%)"
        for _, cmd in ipairs(cmds) do
            cmd_str = cmd_str..cmd.."\n"..check_result.."\n"
        end

        -- write cmd file
        local cmd_name = _normalize_cmd_name(cmd_name)
        local cmd_file_name = path.join(_task_cmd_dir, cmd_name..".bat")
        if #cmd_str > 0 then
            os.rm(cmd_file_name)
            io.writefile(cmd_file_name, cmd_str)
            return cmd_file_name
        end
    end
    
    return nil
end
-- @return: task object
function _combine_task_json(cmds, cmd_name)
    local cmd_file_name = _generate_cmd_file(cmds, cmd_name)
    if cmd_file_name then
        return {
            label = cmd_name,
            type = "shell",
            command = cmd_file_name,
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
        }
    end
    return nil
end

-- generate launches
-- @return: combined natvis file path
function _generate_natvis_files(natvis_files, cmd_name)
    if natvis_files and #natvis_files > 0 then
        local out_put_file_name = path.join(_task_cmd_dir, cmd_name..".natvis")
        
        -- combine commands
        local command = {
            _merge_natvis_script_path,
            "-o", out_put_file_name,
        }
        for _, natvis_file in ipairs(natvis_files) do
            table.insert(command, natvis_file)
        end

        -- run command
        local out, err = os.iorunv(_python, command)

        -- dump output
        if option.get("verbose") then
            if out and #out > 0 then
                print("=====================["..cmd_name.." merge natvis output]=====================")
                printf(out)
                print("=====================["..cmd_name.." merge natvis output]=====================")
            end
        end
        if err and #err > 0 then
            print("=====================["..cmd_name.." merge natvis error]=====================")
            printf(err)
            print("=====================["..cmd_name.." merge natvis error]=====================")
        end

        return out_put_file_name
    end
    return nil
end
-- @return: launch object
function _combine_launch_json(launch_data, pre_task_json, post_task_json)
    local natvis_file_name = _generate_natvis_files(launch_data.natvis_files, launch_data.cmd_name)
    return {
        name = "▶️"..launch_data.label,
        type = "cppvsdbg",
        request = "launch",
        program = launch_data.program,
        args = json.mark_as_array(launch_data.args),
        stopAtEntry = false,
        cwd = launch_data.cwd,
        environment = json.mark_as_array(launch_data.envs),
        console = "integratedTerminal",
        visualizerFile = path.join("${workspaceFolder}", natvis_file_name),
        preLaunchTask = pre_task_json and pre_task_json.label or nil,
        postLaunchTask = post_task_json and post_task_json.label or nil,
        autoAttachChildProcess = true,
    }
end
function _append_launches_and_tasks(launch_data, out_launches_json, out_tasks_json)
    local pre_task_json = _combine_task_json(launch_data.pre_cmds, "pre_"..launch_data.cmd_name)
    local post_task_json = _combine_task_json(launch_data.post_cmds, "post_"..launch_data.cmd_name)
    local launch_json = _combine_launch_json(launch_data, pre_task_json, post_task_json)
    
    table.insert(out_launches_json, launch_json)
    if pre_task_json then
        table.insert(out_tasks_json, pre_task_json)
    end
    if post_task_json then
        table.insert(out_tasks_json, post_task_json)
    end
end
-- @return: attach launch object
function _combine_attach_launch_json()
    -- TODO. natvis files
    return {
        name = "🔍Attach",
        type = "cppvsdbg",
        request = "attach",
        processId = "${command:pickProcess}",
        -- visualizerFile = path.join("${workspaceFolder}", natvis_file_name),
    }
end

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

    -- collect launches data
    local launches_data = {}
    do
        function _add_target(target)
            local proxy_launches_data = _load_launches_from_proxy_target(target, build_dir)
            local binary_launch_data = _load_launch_from_binary_target(target, build_dir)
            if proxy_launches_data then
                for _, data in ipairs(proxy_launches_data) do
                    table.insert(launches_data, data)
                end
            elseif binary_launch_data then
                table.insert(launches_data, binary_launch_data)
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

    -- generate launches and tasks
    local launches_json = {}
    local tasks_json = {}
    for _, launch_data in ipairs(launches_data) do
        _append_launches_and_tasks(launch_data, launches_json, tasks_json)
    end

    -- append attach launch
    table.insert(launches_json, _combine_attach_launch_json())

    -- save launches and tasks
    -- save debug configurations
    json.savefile(".vscode/launch.json", {
        version = "0.2.0",
        configurations = launches_json
    })
    json.savefile(".vscode/tasks.json", {
        version = "2.0.0",
        tasks = tasks_json
    })
end
