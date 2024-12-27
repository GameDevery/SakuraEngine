
------------------------dirs------------------------
function saved_config_dir()
    return "build/.skr/project.conf"
end

function binary_dir()
    return vformat("$(buildir)/$(os)/$(arch)/$(mode)")
end

------------------------find------------------------
function find_program_in_paths(program_name, paths, opt)
    opt = opt or {}

    -- bad case
    if path.is_absolute(program_name) then
        raise("program name cannot be an absolute path")
    end


    -- find program
    for _, _path in ipairs(paths) do
        -- format path
        _path = path.join(vformat(_path), program_name)

        -- check path
        if os.isexec(_path) then
            return _path
        end
    end
end

function find_program_in_system(program_name, opt)
    opt = opt or {}

    -- load opt
    local use_which = opt.use_sys_find or true;
    local use_env_find = opt.use_env_find or true;

    -- helpers
    local _format_program_name_for_winos = function (name)
        local name = program_name:lower()
        if not name:endswith(".exe") then
            name = name .. ".exe"
        end
        return name
    end
    local _check_program_path = function (path)
        if path then
            path = path:trim()
            if os.isexec(path) then
                return path
            end
        end
    end

    -- find use sys find
    if use_which then
        if os.host() == "windows" then -- use where
            -- format program name    
            local find_name = _format_program_name_for_winos(program_name)
            
            -- find
            local found_paths = os.iorunv("where.exe", {find_name})
            if found_paths then
                for _, path in ipairs(found_paths:split("\n")) do
                    path = path:trim()
                    if #path > 0 then
                        if os.isexec(path) then
                            return path
                        end
                    end
                end
            end

        else -- use which
            local program_path = os.iorunv("which", {program_name})
            program_path = _check_program_path(program_path)
            if program_path then
                return program_path
            end
        end
    end

    -- find env find
    if use_env_find then
        if os.host() == "windows" then -- use registry
            local find_name = _format_program_name_for_winos(program_name)
            local program_path = winos.registry_query("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\" .. find_name)
            program_path = _check_program_path(program_path)
            if program_path then
                return program_path
            end
            
        else -- use sys default path
            local paths = {
                "/usr/local/bin",
                "/usr/bin",
            }
            local path = find_program_in_paths(program_name, paths, opt)
            if path then
                return path
            end
        end
    end
end
