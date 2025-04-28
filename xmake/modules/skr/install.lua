import("utils.archive")
import("skr.utils")
import("core.project.project")
import("core.base.object")

-- TODO. Installer 用于 download package 的 custom install 和 in_package_install
------------------------installer------------------------
-- fields:
--   pkg_dir: unpacked package dir
--   bin_dir: build binary dir
--   print_log: need print log
local Installer = Installer or object {}
-- custom copy
-- @param src: in package dir
-- @param dst: out dir
-- @param opt: options
--   opt.rootdir: in package dir, used for copy keep structure
--   opt.dst_is_dir: if dst is a dir, we need create dir first
function Installer:cp(src, dst, opt)
    opt = opt or {}

    -- absolute path
    local abs_src = path.join(self.pkg_dir, src)
    local abs_rootdir = opt.rootdir and path.join(self.pkg_dir, opt.rootdir) or nil

    -- rm dst
    if opt.rm_dst then
        if os.exists(dst) then
            if self.print_log then
                print("remove %s", dst)
            end
            os.rm(dst)
        end
    end

    -- create dir
    if opt.dst_is_dir then
        if not os.isdir(dst) then
            if self.print_log then
                print("create %s", dst)
            end
            os.mkdir(dst)
        end
    end

    -- do cp
    if self.print_log then
        print("copy %s to %s", abs_src, dst)
    end
    os.cp(abs_src, dst, {rootdir = abs_rootdir})
end
-- copy into target binary dir
-- @param src: in package dir
-- @param opt: options
--   opt.rootdir: in package dir, used for copy keep structure
function Installer:bin(src, opt)
    opt = opt or {}

    -- absolute path
    local abs_src = path.join(self.pkg_dir, src)
    local abs_rootdir = opt.rootdir and path.join(self.pkg_dir, opt.rootdir) or nil

    -- do cp
    if self.print_log then
        print("copy %s to %s", abs_src, self.bin_dir)
    end
    os.cp(abs_src, self.bin_dir, {rootdir = abs_rootdir})
end

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
function install_pkg_tool(opt)
    local package_path = utils.find_download_package_tool(opt.name)
    if not package_path then
        raise("tool %s not found", opt.name)
    end

    -- extract package
    local package_file_name = path.filename(package_path)
    utils.on_changed(function (change_info)
        _log(opt, "tool.install", "path \"%s\"", package_path)
        archive.extract(package_path, utils.install_dir_tool())

        -- call after_install
        if opt.after_install then
            utils.redirect_fenv(opt.after_install, install_pkg_tool)
            opt.after_install()
        end
    end, {
        cache_file = utils.depend_file_install("tools/"..package_file_name), 
        files = {package_path},
        force = opt.force,
    })
end
function install_pkg_resources(opt)
    local package_path = utils.find_download_package_sdk(opt.name, opt)
    if not package_path then
        raise("resources %s not found", opt.name)
    end
    local temp_dir = utils.install_temp_dir(package_path)
    local package_file_name = path.filename(package_path)
    local depend_file_name = package_file_name..(opt.trigger_target or "")
    local extract_exist = os.isdir(temp_dir)

    -- extract
    utils.on_changed(function (change_info)
        _log(opt, "resource.extract", "temp_dir \"%s\"", temp_dir)
        if extract_exist then
            os.rmdir(temp_dir)
        end
        archive.extract(package_path, temp_dir)
    end, {
        cache_file = utils.depend_file_install("resources/extract/"..depend_file_name),
        files = {package_path},
        force = not extract_exist or opt.force,
    })

    -- copy
    utils.on_changed(function (change_info)
        _log(opt, "resource.copy", "path \"%s\"", package_path)
        if not os.isdir(opt.out_dir) then
            os.mkdir(opt.out_dir)
        end
        os.cp(path.join(temp_dir, "**"), opt.out_dir, {rootdir = temp_dir})
        
        -- call after_install
        if opt.after_install then
            utils.redirect_fenv(opt.after_install, install_pkg_resources)
            opt.after_install()
        end
    end, {
        cache_file = utils.depend_file_install("resources/copy/"..depend_file_name), 
        files = {package_path},
        force = not extract_exist or opt.force,
    })
end
function install_pkg_sdk(opt)
    local package_path = utils.find_download_package_sdk(opt.name, opt)
    if not package_path then
        raise("sdk %s not found", opt.name)
    end
    local temp_dir = utils.install_temp_dir(package_path)
    local package_file_name = path.filename(package_path)
    local extract_exist = os.isdir(temp_dir)

    -- extract
    utils.on_changed(function (change_info)
        _log(opt, "sdk.extract", "temp_dir \"%s\"", temp_dir)
        if extract_exist then
            os.rmdir(temp_dir)
        end
        archive.extract(package_path, temp_dir)
    end, {
        cache_file = utils.depend_file_install("sdk/extract/"..package_file_name),
        files = { package_path },
        force = not extract_exist or opt.force
    })

    -- copy
    utils.on_changed(function (change_info)
        _log(opt, "sdk.copy", "path \"%s\"", package_path)
        if not os.isdir(opt.out_dir) then
            os.mkdir(opt.out_dir)
        end
        if os.host() == "windows" then
            os.cp(path.join(temp_dir, "**.lib"), opt.out_dir)
            os.cp(path.join(temp_dir, "**.dll"), opt.out_dir)
        elseif os.host() == "macosx" then
            os.cp(path.join(temp_dir, "**.dylib"), opt.out_dir)
            os.cp(path.join(temp_dir, "**.a"), opt.out_dir)
        end

        -- call after_install
        if opt.after_install then
            utils.redirect_fenv(opt.after_install, install_pkg_sdk)
            opt.after_install()
        end
    end, {
        cache_file = utils.depend_file_install("sdk/copy/"..package_file_name),
        files = { package_path },
        force = not extract_exist or opt.force,
    })
end
function install_pkg_file(opt)
    local file_path = utils.find_download_file(opt.name)
    if not file_path then
        raise("file %s not found", opt.name)
    end
    local depend_file_name = opt.name..(opt.trigger_target and "_"..opt.trigger_target or "")

    utils.on_changed(function (change_info)
        _log(opt, "download_file.copy", "path \"%s\"", file_path)
        if not os.isdir(opt.out_dir) then
            os.mkdir(opt.out_dir)
        end
        os.cp(file_path, opt.out_dir)

        -- call after_install
        if opt.after_install then
            utils.redirect_fenv(opt.after_install, install_pkg_file)
            opt.after_install()
        end
    end, {
        cache_file = utils.depend_file_install("files/"..depend_file_name), 
        files = { file_path },
        force = opt.force,
    })
end
function install_pkg_custom(opt)
    local package_path = utils.find_download_package_sdk(opt.name, opt)
    if not package_path then
        raise("package %s not found", opt.name)
    end
    local temp_dir = utils.install_temp_dir(package_path)
    local package_file_name = path.filename(package_path)
    local extract_exist = os.isdir(temp_dir)

    -- extract
    utils.on_changed(function (change_info)
        _log(opt, "pkg_custom.extract", "temp_dir \"%s\"", temp_dir)
        if extract_exist then
            os.rmdir(temp_dir)
        end
        archive.extract(package_path, temp_dir)
    end, {
        cache_file = utils.depend_file_install("pkg_custom/extract/"..package_file_name),
        files = { package_path },
        force = not extract_exist or opt.force
    })

    -- do install
    utils.on_changed(function (change_info)
        _log(opt, "pkg_custom.install", "path \"%s\"", package_path)
        if not os.isdir(opt.out_dir) then
            os.mkdir(opt.out_dir)
        end

        -- call custom install
        utils.redirect_fenv(opt.custom_install, install_pkg_custom)
        local installer = Installer {
            pkg_dir = temp_dir,
            bin_dir = opt.out_dir,
        }
        opt.custom_install(installer)

        -- call after_install
        if opt.after_install then
            utils.redirect_fenv(opt.after_install, install_pkg_custom)
            opt.after_install()
        end
    end, {
        cache_file = utils.depend_file_install("sdk/copy/"..package_file_name),
        files = { package_path },
        force = not extract_exist or opt.force,
    })
end
function install_pkg_bound(opt)
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
    utils.on_changed(function (change_info)
        _log(opt, "file.copy", "%d files", #files)
        if not os.isdir(opt.out_dir) then
            os.mkdir(opt.out_dir)
        end
        for _, file in ipairs(files) do
            os.cp(file, opt.out_dir, {rootdir = opt.root_dir})
        end
    end, {
        cache_file = utils.depend_file_install("copy_files/"..opt.name),
        files = files,
        force = opt.force,
    })
end
function install_custom(opt)
    utils.on_changed(function (change_info)
        _log(opt, "custom")
        utils.redirect_fenv(opt.func, install_custom)
        opt.func()
    end, {
        cache_file = utils.depend_file_install("custom/"..opt.name),
        files = {path.join(os.scriptdir(), "install.lua")}, -- TODO. use flag file
        force = opt.force,
    })
end
-- opt see xmake/rules/install.lua
function install_from_kind(kind, opt)
    if kind == "download" then
        if opt.install_func == "tool" then
            install_pkg_tool(opt)
        elseif opt.install_func == "resources" then
            install_pkg_resources(opt)
        elseif opt.install_func == "sdk" then
            install_pkg_sdk(opt)
        elseif opt.install_func == "file" then
            install_pkg_file(opt)
        elseif opt.install_func == "custom" then
            install_pkg_custom(opt)
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
    opt.out_dir = opt.out_dir or path.absolute(target:targetdir())
    
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
                if not _filter(item.opt.toolchain, target:toolchains()[1]:name()) then
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