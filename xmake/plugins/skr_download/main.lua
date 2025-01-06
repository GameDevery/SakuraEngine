import("core.base.option")
import("core.base.task")
import("core.base.global")
import("core.project.config")
import("core.project.project")
import("core.project.depend")
import("core.platform.platform")
import("core.base.json")
import("skr.utils")
import("skr.download")
import("skr.install")

function main()
    -- load config
    utils.load_config()
    -- load targets
    project.load_targets()

    -- get options
    local opt_kind = option.get("kind")
    local opt_force = option.get("force")
    local opt_debug = option.get("debug")
    local opt_packages = option.get("packages")

    -- check options
    if opt_kind then
        local kinds = {"tool", "resources", "sdk", "file"}
        if not table.contains(kinds, opt_kind) then
            raise("invalid kind: %s", opt_kind)
        end
    end
    if opt_packages then
        if not opt_kind then
            raise("please specify kind when download specific packages")
        end
    end

    -- solve packages
    local packages = {}
    if opt_packages then
        for _, package in ipairs(opt_packages) do
            table.insert(packages, {
                name = package,
                kind = opt_kind,
                debug = opt_debug,
            })
        end
    else
        local install_items = install.collect_install_items_from_rules()
        for _, item in ipairs(install_items) do
            -- filter install kind
            if item.kind ~= "download" then
                goto continue
            end

            local opt = item.opt
            
            -- filter install func
            if opt_kind and opt.install_func ~= opt_kind then
                goto continue
            end

            -- filter debug
            if opt_debug ~= nil and opt.debug ~= opt_debug then
                goto continue
            end

            -- filter disabled
            if not item.trigger_target:is_default() then
                -- print("disabled %s", item.trigger_target:name())
                goto continue
            end
            
            -- add package
            if item.kind == "download" then
                local opt = item.opt
                if not opt_kind or opt.install_func == opt_kind then
                    table.insert(packages, {
                        name = opt.name,
                        kind = opt.install_func,
                        debug = opt.debug,
                    })
                end
            end

            ::continue::
        end
    end

    -- init download
    -- TODO. config source from options
    local download_tool = download.new()
    download_tool:add_source_default()
    download_tool:fetch_manifests()

    -- download packages
    for _, package in ipairs(packages) do
        if package.kind == "tool" then
            download_tool:download_tool(package.name, {force = opt_force})
        elseif package.kind == "resources" then
            download_tool:download_file(package.name, {force = opt_force})
        elseif package.kind == "sdk" then
            download_tool:download_sdk(package.name, {force = opt_force, debug = package.debug})
        elseif package.kind == "file" then
            download_tool:download_file(package.name, {force = opt_force})
        end
    end
end