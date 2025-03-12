rule("skr.install")
    before_build(function (target)
        import("skr.utils")
        import("skr.install")
        import("utils.archive")

        if not target:is_default() then
            return
        end

        -- help functions
        local function _filter(t, v)
            if t then -- do filter
                return t == v or table.contains(t, v)
            else -- no filter means pass
                return true
            end
        end

        -- get config
        local install_items = target:values("skr.install")
        if not install_items then
            return
        end
        install_items = table.wrap(install_items)

        -- filter install items
        local download_items = {}
        local files_items = {}
        local custom_items = {}
        for _, item in ipairs(install_items) do
            -- check opts
            if not item.opt then
                raise ("item without opt: %s", item)
            end

            -- global filter
            if not _filter(item.opt.plat, target:plat()) then
                goto continue
            end
            if not _filter(item.opt.arch, os.arch()) then
                goto continue
            end
            if not _filter(item.opt.toolchain, target:toolchains()[1]:name()) then
                goto continue
            end
            
            -- add items
            if item.kind == "download" then
                table.insert(download_items, item)
            elseif item.kind == "files" then
                table.insert(files_items, item)
            elseif item.kind == "custom" then
                table.insert(custom_items, item)
            else
                raise("invalid install kind: %s, table:\n%s", item.kind, item)
            end
            
            ::continue::
        end

        -- install packages
        for _, item in ipairs(download_items) do
            install.fill_opt(item.opt, target)
            install.install_from_kind(item.kind, item.opt)
        end

        -- install files
        for _, item in ipairs(files_items) do
            install.fill_opt(item.opt, target)
            install.install_from_kind(item.kind, item.opt)
        end

        -- install custom items
        for _, item in ipairs(custom_items) do
            install.install_from_kind(item.kind, item.opt)
        end
    end)
rule_end()