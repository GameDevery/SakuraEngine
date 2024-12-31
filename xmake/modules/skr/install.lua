import("utils.archive")
import("skr.utils")
import("core.project.depend")
import("core.base.object")

------------------------concepts------------------------
-- source: {
--   name: [string] source name
--   url: [string] base url
-- }

local install = install or object {}

function new()
    return install { sources = {} }
end

------------------------install sources------------------------
function install:add_source(name, path)
    if not name or not path then
        raise("invalid source: %s, %s", name, path)
    end

    if self.sources[name] then
        raise("source %s already exists", name)
    end

    self.sources[name] = path
end
function install:remove_source(name)
    if not name then
        return
    end

    self.sources[name] = nil
end
function install:add_source_default()
    self:add_source("skr_download", utils.download_dir())
end

------------------------install------------------------
-- TODO. force install
function install_tool(opt)
    local package_path = utils.find_download_package_tool(opt.name)
    if not package_path then
        raise("tool %s not found", opt.name)
    end

    -- extract package
    local package_file_name = path.filename(package_path)
    depend.on_changed(function ()
        archive.extract(package_path, utils.install_dir_tool())
    end, {
        dependfile = utils.depend_file("install/tools/"..package_file_name),
        lastmtime = os.mtime(package_path),
        files = {package_path},
    })
end
function install_resources(opt)
    local package_path = utils.find_download_package_sdk(opt.name)
    if not package_path then
        raise("resources %s not found", opt.name)
    end
    local temp_dir = utils.install_temp_dir(package_path)
    local package_file_name = path.filename(package_path)
    
    -- extract
    depend.on_changed(function ()
        archive.extract(package_path, temp_dir)
    end, {
        dependfile = utils.depend_file("install/resources/"..package_file_name),
        lastmtime = os.mtime(package_path),
        files = { package_path },
    })

    -- copy
    depend.on_changed(function ()
        os.cp(path.join(temp_dir, "**"), opt.out_dir, {rootdir = temp_dir})
    end, {
        dependfile = utils.depend_file("install/resources/"..package_file_name..".copy"),
        lastmtime = os.mtime(package_path),
        files = { package_path },
    })
end
function install_sdk(opt)
    local package_path = utils.find_download_package_sdk(opt.name)
    if not package_path then
        raise("sdk %s not found", opt.name)
    end
    local temp_dir = utils.install_temp_dir(package_path)
    local package_file_name = path.filename(package_path)
    
    -- extract
    depend.on_changed(function ()
        archive.extract(package_path, temp_dir)
    end, {
        dependfile = utils.depend_file("install/sdk/"..package_file_name),
        lastmtime = os.mtime(package_path),
        files = { package_path },
    })

    -- copy
    depend.on_changed(function ()
        if os.host() == "windows" then
            os.cp(path.join(temp_dir, "**.lib"), opt.out_dir)
            os.cp(path.join(temp_dir, "**.dll"), opt.out_dir)
        elseif os.host() == "macosx" then
            os.cp(path.join(temp_dir, "**.dylib"), opt.out_dir)
            os.cp(path.join(temp_dir, "**.a"), opt.out_dir)
        end
    end, {
        dependfile = utils.depend_file("install/sdk/"..package_file_name..".copy"),
        lastmtime = os.mtime(package_path),
        files = { package_path },
    })
end
function install_file(opt)
    local file_path = utils.find_download_file(opt.name)
    if not file_path then
        raise("file %s not found", opt.name)
    end

    depend.on_changed(function ()
        os.cp(file_path, opt.out_dir)
    end, {
        dependfile = utils.depend_file("install/files/"..opt.name),
        lastmtime = os.mtime(file_path),
        files = { file_path },
    })
end
function copy_files(opt)
    -- grab files
    local files = {}
    for _, file in ipairs(opt.files) do
        local grab_files = os.files(file)
        table.append(files, table.unpack(grab_files))
    end

    -- copy
    depend.on_changed(function ()
        os.cp(files, opt.out_dir, {rootdir = opt.root_dir})
    end, {
        dependfile = utils.depend_file("install/copy_files/"..opt.name),
        files = files,
    })
end
-- opt see xmake/rules/install.lua
function install(kind, opt)
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
    end
end


-- TODO. clean up