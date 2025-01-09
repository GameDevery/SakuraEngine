import("lib.detect")
import("core.project.config")

------------------------dirs------------------------
function skr_build_artifact_dir()
    return "build/.skr/"
end
function skr_codegen_dir(target_name)
    return path.join(skr_build_artifact_dir(), "codegen", target_name)
end
function saved_config_path()
    return path.join(skr_build_artifact_dir(), "project.config")
end
function binary_dir()
    return vformat("$(buildir)/$(os)/$(arch)/$(mode)")
end
function download_dir()
    return path.join(skr_build_artifact_dir(), "download")
end
function install_dir_tool()
    -- TODO. no setup.lua
    -- return path.join(skr_build_artifact_dir(), vformat("tool/$(host)/$(arch)"))
    return path.join(skr_build_artifact_dir(), vformat("tool/$(host)"))
end
function install_dir_sdk(target)
    return target:targetdir()
end
function install_temp_dir(package_path)
    local base_name = path.basename(package_path)
    return path.join(skr_build_artifact_dir(), "install", base_name)
end
function depend_file(sub_path)
    return path.join(skr_build_artifact_dir(), "deps", sub_path..".d")
end
function flag_file(sub_path)
    return path.join(skr_build_artifact_dir(), "flags", sub_path..".flag")
end
function log_file(sub_path)
    return path.join(skr_build_artifact_dir(), "log", sub_path..".log")
end

------------------------config------------------------
function load_config()
    if not os.isfile(saved_config_path()) then
        raise("config file not found: %s", saved_config_path())
    end
    config.load(saved_config_path(), {force=true})
end
function save_config()
    config.save(saved_config_path(), {public=true})
end

------------------------package name rule------------------------
function package_name_tool(tool_name)
    return vformat("%s-$(host)-%s.zip", tool_name, os.arch())
end
function package_name_sdk(sdk_name, opt)
    opt = opt or {}

    -- TODO. $(host) -> $(os)
    if opt.debug then
        return vformat("%s_d-$(host)-%s.zip", sdk_name, os.arch())
    else
        return vformat("%s-$(host)-%s.zip", sdk_name, os.arch())
    end
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
function find_download_file(file_name)
    return detect.find_file(file_name, download_dir())
end
function find_download_package_tool(tool_name)
    return find_download_file(package_name_tool(tool_name))
end
function find_download_package_sdk(sdk_name, opt)
    return find_download_file(package_name_sdk(sdk_name, opt))
end
function find_tool(tool_name, sub_paths, opt)
    -- combine paths
    local paths = { install_dir_tool() }
    for _, sub_path in ipairs(sub_paths) do
        table.insert(paths, path.join(install_dir_tool(), sub_path))
    end

    -- find
    return find_program_in_paths(tool_name, paths, opt)
end

------------------------find spec program------------------------
function find_python()
    local embedded_python = find_tool("python", {"python-embed"})
    if embedded_python then
        return embedded_python
    end

    local system_python = find_program_in_system("python")
    if system_python then
        return system_python
    end
end
function find_meta()
    local meta = find_tool("meta")
end

------------------------tools------------------------
-- solve change info for files
-- @param cache_file: cache_file used to save time_stamp
-- @param files: files to check
-- @param opt: options:
--   check_sha256: use sha256 to check file contents, this is useful when file time_stamp is not reliable
--   flag_files: 
--     file: flag file name
-- @return: 
--   [0] is any file changed
--   [1] change info: {
--     files = {
--       file = changed file,
--       reason = "new"|"modify", -- new means add to cache_file, modify means mtime miss match or sha256 miss match, flag means flag file changed
--     }
--     deleted_files = list of deleted files,
--     flag_files = same as files,
--     deleted_flag_files = same as deleted_files,
--   }
function is_changed(cache_file, files, opt)
    -- load opt
    opt = opt or {}
    local check_sha256 = opt.check_sha256 or false
    local flag_files = opt.flag_files or nil
    
    -- load cache file
    if not os.exists(cache_file) then
        -- all files are new
        local change_infos = {}
        for _, file in ipairs(files) do
            table.insert(change_infos, {
                file = file,
                reason = "new"
            })
        end

        -- save cache
        local cache = {
            files = {},
            flag_files = {},
        }
        for _, file in ipairs(files) do
            if check_sha256 then
                cache.files[file] = {
                    sha256 = hash.sha256(file),
                }
            else
                cache.files[file] = {
                    mtime = os.mtime(file),
                }
            end
        end
        if flag_files then
            for _, file in ipairs(flag_files) do
                if check_sha256 then
                    cache.flag_files[file] = {
                        sha256 = hash.sha256(file),
                    }
                else
                    cache.flag_files[file] = {
                        mtime = os.mtime(file),
                    }
                end
            end
        end
        io.save(cache_file, cache)

        return true, change_infos, {}
    end
    cache = io.load(cache_file)

    local _check_file = function (cache_info, file)
        -- check cache info
        if not cache_info then
            return "new", nil, nil
        end

        -- check sha256
        if check_sha256 then
            local sha = hash.sha256(file)
            if sha ~= cache_info.sha256 then
                return "modify", nil, sha
            end
        else
            local mtime = os.mtime(file)
            if mtime ~= cache_info.mtime then
                return "modify", mtime, nil
            end
        end
    end
    local _check_cache_and_update = function (cache_tbl, files)
        -- check files
        local checked_files_info = {}
        for _, file in ipairs(files) do
            local reason, mtime, sha = _check_file(cache_tbl[file], file)
            
            -- record checked info
            checked_files_info[file] = {
                reason = reason,
            }

            -- update cache
            if reason then
                cache_tbl[file] = {
                    mtime = mtime or os.mtime(file),
                    sha256 = sha,
                }
            end
        end

        -- check deleted files
        local deleted_files = {}
        for file, info in pairs(cache_tbl) do
            if not checked_files_info[file] then
                -- record deleted info
                table.insert(deleted_files, file)
                
                -- remove from cache
                cache_tbl[file] = nil
            end
        end

        return checked_files_info, deleted_files
    end

    -- check
    local checked_files_info, deleted_files = _check_cache_and_update(cache.files, files)
    local checked_flag_files_info, deleted_flag_files = _check_cache_and_update(cache.flag_files, flag_files)

    -- save cache
    io.save(cache_file, cache)

    -- build final change info
    local change_info = {
        files = {},
        deleted_files = deleted_files,
        flag_files = {},
        deleted_flag_files = deleted_flag_files,
        dump = function(self)
            for _, file in ipairs(self.files) do
                if file.reason == "new" then
                    cprint("${green}[%s]${clear} %s", file.reason, file.file)
                else
                    cprint("${cyan}[%s]${clear} %s", file.reason, file.file)
                end
            end
            for _, file in ipairs(self.deleted_files) do
                cprint("${red}[%s]${clear} %s", "delete", file)
            end
            for _, file in ipairs(self.flag_files) do
                if file.reason == "new" then
                    cprint("${green}[%s]${clear} %s", file.reason, file.file)
                else
                    cprint("${cyan}[%s]${clear} %s", file.reason, file.file)
                end
            end
            for _, file in ipairs(self.deleted_flag_files) do
                cprint("${red}[%s]${clear} %s", "delete", file)
            end
        end,
        is_file_changed = function(self)
            return #self.files > 0 or #self.deleted_files > 0
        end,
        is_flag_changed = function(self)
            return #self.flag_files > 0 or #self.deleted_flag_files > 0
        end
    }
    for file, info in pairs(checked_files_info) do
        if (info.reason) then
            table.insert(change_info.files, {
                file = file,
                reason = info.reason,
            })
        end
    end
    for file, info in pairs(checked_flag_files_info) do
        if (info.reason) then
            table.insert(change_info.flag_files, {
                file = file,
                reason = info.reason,
            })
        end
    end

    local changed = change_info:is_file_changed() or change_info:is_flag_changed()

    return changed, change_info
end
function write_flag_file(sub_path)
    local path = flag_file(sub_path)
    os.writefile(path, "FLAG FILE FOR TRIGGER [utils.is_changed]")
end

------------------------lua utils------------------------
function setfenv(fn, env)
    local i = 1
    while true do
        local name = debug.getupvalue(fn, i)
        if name == "_ENV" then
        debug.upvaluejoin(fn, i, (function()
            return env
        end), 1)
        break
        elseif not name then
        break
        end

        i = i + 1
    end

    return fn
end
function getfenv(fn)
    local i = 1
    while true do
        local name, val = debug.getupvalue(fn, i)
        if name == "_ENV" then
            return val
        elseif not name then
            break
        end
        i = i + 1
    end
end
function redirect_fenv(fn, reference_fn)
    setfenv(fn, getfenv(reference_fn))
end