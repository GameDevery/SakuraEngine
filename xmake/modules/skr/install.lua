import("utils.archive")
import("skr.utils")
import("core.project.project")
import("core.project.depend")
import("core.base.object")

------------------------log tools------------------------
function _log(opt, type, format, ...)
    format = format or ""
    cprint("${green}[%s](%s): %s${clear}. "..format, 
        opt.trigger_target or "GLOBAL",
        type,
        opt.name,
        ...);
end

------------------------install------------------------
function install_tool(opt)
    local package_path = utils.find_download_package_tool(opt.name)
    if not package_path then
        raise("tool %s not found", opt.name)
    end

    -- extract package
    local package_file_name = path.filename(package_path)
    depend.on_changed(function ()
        _log(opt, "tool.install", "path \"%s\"", package_path)
        archive.extract(package_path, utils.install_dir_tool())

        -- call after_install
        if opt.after_install then
            utils.redirect_fenv(opt.after_install, install_tool)
            opt.after_install()
        end
    end, {
        dependfile = utils.depend_file("install/tools/"..package_file_name),
        lastmtime = os.mtime(package_path),
        files = {package_path},
        changed = opt.force,
    })
end
function install_resources(opt)
    local package_path = utils.find_download_package_sdk(opt.name)
    if not package_path then
        raise("resources %s not found", opt.name)
    end
    local temp_dir = utils.install_temp_dir(package_path)
    local package_file_name = path.filename(package_path)
    local extract_exist = os.isdir(temp_dir)

    -- extract
    depend.on_changed(function ()
        _log(opt, "resource.extract", "temp_dir \"%s\"", temp_dir)
        if extract_exist then
            os.rmdir(temp_dir)
        end
        archive.extract(package_path, temp_dir)
    end, {
        dependfile = utils.depend_file("install/resources/extract/"..package_file_name),
        lastmtime = os.mtime(package_path),
        files = { package_path },
        changed = not extract_exist or opt.force,
    })

    -- copy
    depend.on_changed(function ()
        _log(opt, "resource.copy", "path \"%s\"", package_path)
        os.cp(path.join(temp_dir, "**"), opt.out_dir, {rootdir = temp_dir})
        
        -- call after_install
        if opt.after_install then
            utils.redirect_fenv(opt.after_install, install_resources)
            opt.after_install()
        end
    end, {
        dependfile = utils.depend_file("install/resources/copy/"..package_file_name),
        lastmtime = os.mtime(package_path),
        files = { package_path },
        changed = not extract_exist or opt.force,
    })
end
function install_sdk(opt)
    local package_path = utils.find_download_package_sdk(opt.name)
    if not package_path then
        raise("sdk %s not found", opt.name)
    end
    local temp_dir = utils.install_temp_dir(package_path)
    local package_file_name = path.filename(package_path)
    local extract_exist = os.isdir(temp_dir)

    -- extract
    depend.on_changed(function ()
        _log(opt, "sdk.extract", "temp_dir \"%s\"", temp_dir)
        if extract_exist then
            os.rmdir(temp_dir)
        end
        archive.extract(package_path, temp_dir)
    end, {
        dependfile = utils.depend_file("install/sdk/extract/"..package_file_name),
        lastmtime = os.mtime(package_path),
        files = { package_path },
        changed = not extract_exist or opt.force,
    })

    -- copy
    depend.on_changed(function ()
        _log(opt, "sdk.copy", "path \"%s\"", package_path)
        if os.host() == "windows" then
            os.cp(path.join(temp_dir, "**.lib"), opt.out_dir)
            os.cp(path.join(temp_dir, "**.dll"), opt.out_dir)
        elseif os.host() == "macosx" then
            os.cp(path.join(temp_dir, "**.dylib"), opt.out_dir)
            os.cp(path.join(temp_dir, "**.a"), opt.out_dir)
        end

        -- call after_install
        if opt.after_install then
            utils.redirect_fenv(opt.after_install, install_sdk)
            opt.after_install()
        end
    end, {
        dependfile = utils.depend_file("install/sdk/copy/"..package_file_name),
        lastmtime = os.mtime(package_path),
        files = { package_path },
        changed = not extract_exist or opt.force,
    })
end
function install_file(opt)
    local file_path = utils.find_download_file(opt.name)
    if not file_path then
        raise("file %s not found", opt.name)
    end

    depend.on_changed(function ()
        _log(opt, "download_file.copy", "path \"%s\"", file_path)
        os.cp(file_path, opt.out_dir)

        -- call after_install
        if opt.after_install then
            utils.redirect_fenv(opt.after_install, install_file)
            opt.after_install()
        end
    end, {
        dependfile = utils.depend_file("install/files/"..opt.name),
        lastmtime = os.mtime(file_path),
        files = { file_path },
        changed = opt.force,
    })
end
function copy_files(opt)
    -- check opt
    if not opt.out_dir then
        raise("out_dir not set")
    end
    if not opt.files then
        raise("files not set")
    end

    -- grab files
    local files = {}
    for _, file in ipairs(opt.files) do
        local grab_files = os.files(file)
        table.append(files, table.unpack(grab_files))
    end

    -- copy
    depend.on_changed(function ()
        _log(opt, "file.copy", "%d files", #files)
        if not os.isdir(opt.out_dir) then
            os.mkdir(opt.out_dir)
        end
        for _, file in ipairs(files) do
            os.cp(file, opt.out_dir, {rootdir = opt.root_dir})
        end
    end, {
        dependfile = utils.depend_file("install/copy_files/"..opt.name),
        files = files,
        changed = opt.force,
    })
end
function install_custom(opt)
    depend.on_changed(function ()
        _log(opt, "custom")
        utils.redirect_fenv(opt.func, install_custom)
        opt.func()
    end, {
        dependfile = utils.depend_file("install/custom/"..opt.name),
        files = { os.scriptdir() }, -- just for one time run
        changed = opt.force,
    })
end
-- opt see xmake/rules/install.lua
function install_from_kind(kind, opt)
    if kind == "download" then
        if opt.install_func == "tool" then
            install_tool(opt)
        elseif opt.install_func == "resources" then
            install_resources(opt)
        elseif opt.install_func == "sdk" then
            install_sdk(opt)
        elseif opt.install_func == "file" then
            install_file(opt)
        elseif opt.install_func == "custom" then
            raise("custom install not supported yet")
        elseif opt.install_func == nil then
            raise("install in package not supported yet")
        else
            raise("invalid install_func: %s", opt.install_func)
        end
    elseif kind == "files" then
        copy_files(opt)
    elseif kind == "custom" then
        install_custom(opt)
    end
end

------------------------opt helper------------------------
function fill_opt(opt, target)
    if not opt or not target then
        return
    end

    -- handle out_dir
    opt.out_dir = opt.out_dir or target:targetdir()
    
    -- handle trigger_target for log
    opt.trigger_target = target:name()

    -- handle relative paths
    for i, file in ipairs(opt.files) do
        if not path.is_absolute(file) then
            opt.files[i] = path.absolute(file, target:scriptdir())
        end
    end
    if opt.root_dir and not path.is_absolute(opt.root_dir) then
        opt.root_dir = path.absolute(opt.root_dir, target:scriptdir())
    end
    if opt.out_dir and not path.is_absolute(opt.out_dir) then
        opt.out_dir = path.absolute(opt.out_dir, target:targetdir())
    end
end

------------------------add item helper------------------------
function add_rule_if_not_found(target)
    -- add rules
    if not target:rule("skr.install") then
        target:add("rules", "skr.install")
    end
end
function add_install_item(target, kind, opt)
    target:add("values", "skr.install", table.wrap_lock({
        kind = kind,
        opt = opt,
    }), {
        force = true,
        expand = false,
    })
end

------------------------collect from rules------------------------
function collect_install_items()
    local result = {}

    -- filter helper
    local function _filter(t, v)
        if t then -- do filter
            return t == v or table.contains(t, v)
        else -- no filter means pass
            return true
        end
    end

    for _, target in pairs(project.ordertargets()) do
        local install_rule = target:rule("skr.install")
        local install_items = target:values("skr.install")
        install_items = table.wrap(install_items)
        if install_rule and install_items then
            for _, item in ipairs(install_items) do
                -- check opts
                if not item.opt then
                    raise ("item without opt: %s", item)
                end
                
                -- filters
                if not _filter(item.opt.plat, target:plat()) then
                    goto continue
                end
                if not _filter(item.opt.arch, os.arch()) then
                    goto continue
                end

                -- insert
                item.trigger_target = target
                table.insert(result, item)
                ::continue::
            end
        end
    end
    return result
end

-- TODO. clean up