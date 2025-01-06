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
    local opt_mode = option.get("mode")
    local opt_force = option.get("force")
    local opt_list = option.get("list")
    local opt_items = option.get("items")

    -- check options
    if opt_kind then
        local kinds = {"tool", "resources", "sdk", "file"}
        if not table.contains(kinds, opt_kind) then
            raise("invalid kind: %s", opt_kind)
        end
    end
    if opt_mode then
        local modes = {"download", "files"}
        if not table.contains(modes, opt_mode) then
            raise("invalid mode: %s", opt_mode)
        end
    end

    -- solve packages
    local items_info = {}
    local install_items = install.collect_install_items()
    for _, item in ipairs(install_items) do
        -- filter install mode
        if opt_mode and item.kind ~= opt_mode then
            goto continue
        end

        local opt = item.opt

        -- filter install kind
        if item.kind == "download" then
            if opt_kind and opt.install_func ~= opt_kind then
                goto continue
            end
        end
        
        -- filter install items
        if opt_items then
            if not table.contains(opt_items, opt.name) then
                goto continue
            end
        end

        -- filter disabled
        if not item.trigger_target:is_default() then
            -- print("disabled %s", item.trigger_target:name())
            goto continue
        end

        -- insert items
        table.insert(items_info, item)

        ::continue::
    end

    -- list all install items
    if opt_list then
        for _, item in ipairs(install_items) do
            if option.get("verbose") then
                cprint("[%s]${cyan}(%s|%s)${clear}, opt: \n%s"
                    , item.trigger_target:name()
                    , item.kind
                    , item.opt.name
                    , string.serialize(item.opt)
                )
            else
                cprint("[%s]${cyan}(%s|%s)${clear}"
                    , item.trigger_target:name()
                    , item.kind
                    , item.opt.name
                )
            end
        end
        return
    end

    -- install items
    for _, item in ipairs(items_info) do
        item.opt = item.opt or {}
        item.opt.force = opt_force
        install.fill_opt(item.opt, item.trigger_target)
        install.install_from_kind(item.kind, item.opt)
    end
end