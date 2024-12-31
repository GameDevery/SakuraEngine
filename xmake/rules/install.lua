-----------------------ext api-----------------------
-- add install rule
function  skr_install_rule()
    add_rules("skr.install")
end

-- 需求梳理
-- 1. 需要从 download 目录解压并拷贝到目标目录
-- 2. 需要通过路径匹配拷贝到目标目录
-- 3. 需要安装工具到工具目录

--   kind: "download"/"files"，不同的 kind 有不同的参数
--   [kind = any]: 共有参数
--     plat: filter target platform
--     arch: filter target architecture
--   [kind = “download"]:
--     name: package or files name
--     install_func: "tool"/"resources"/"sdk"/"file"/nil, nil means use install.lua in package
--     debug: wants download debug sdk
--     out_dir: override output directory
--   [kind = "files"]:
--     name: install name, for solve depend
--     files: list of files
--     out_dir: output directory
--     root_dir: root directory, nli mean erase directory structure when copy
function skr_install(kind, opt)
    add_values("skr.install", {
        kind = kind,
        opt = opt,
    })
end

-----------------------rules-----------------------
rule("skr.install")
    before_build(function (target)
        import("skr.utils")
        import("skr.install")
        import("core.project.depend")
        import("utils.archive")

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

        -- filter install items
        local download_items = {}
        local files_items = {}
        for _, item in ipairs(install_items) do
            -- check opts
            if not item.opt then
                raise ("item without opt: %s", item)
            end

            -- global filter
            if not _filter(item.opt.plat, target:plat()) then
                goto continue
            end
            if not _filter(item.opt.arch, target:arch()) then
                goto continue
            end
            
            -- add items
            if item.kind == "download" then
                table.insert(download_items, item)
            elseif item.kind == "files" then
                table.insert(files_items, item)
            else
                raise("invalid install kind: %s, table:\n%s", item.kind, item)
            end

            if not item.opt then
                goto continue
            end
            ::continue::
        end

        -- install packages
        for _, item in ipairs(download_items) do
            item.opt.out_dir = item.opt.out_dir or target:targetdir()
            install.install(item.kind, item.opt)
        end

        -- install files
        for _, item in ipairs(files_items) do
            item.opt.out_dir = item.opt.out_dir or target:targetdir()
            install.install(item.kind, item.opt)
        end
    end)
rule_end()