function skr_global_target()
    target("Skr.Global")
end

function skr_global_target_end()
    target_end()
end


skr_global_target()
    -- basic
    set_kind("phony")
    set_policy("build.fence", true)

    -- install rules
    add_rules("skr.install")

    -- scripts
    on_load(function (target)
        import("skr.utils")
        import("skr.analyze")
        import("skr.install")
        import("skr.download")
        
        argv = xmake.argv()

        -- save config
        if argv[1] == "f" or argv[1] == "config" then
            utils.save_config()
        end

        -- trigger analyze
        if analyze.filter_analyze_trigger() then
            analyze.trigger_analyze()
        end
    end)
    on_config(function (target)
        import("skr.utils")
        import("skr.analyze")
        import("skr.install")
        import("skr.download")
        
        -- auto trigger download
        local no_download = get_config("no_download")
        if (not no_download) and (not analyze.in_analyze_phase()) then
            -- load install items
            local install_items = install.collect_install_items()

            -- check download
            local lost_download = {}
            for _, item in ipairs(install_items) do
                if item.kind == "download" then
                    local found_package
                    if item.opt.install_func == "tool" then
                        found_package = utils.find_download_package_tool(item.opt.name)
                    elseif item.opt.install_func == "sdk" then
                        found_package = utils.find_download_package_sdk(item.opt.name, {debug = item.opt.debug})
                    else
                        found_package = utils.find_download_file(item.opt.name)
                    end
                    
                    if not found_package then
                        table.insert(lost_download, item)
                    end
                end
            end
    
            -- download lost packages
            if #lost_download > 0 then
                cprint("${cyan}lost download packages, download now${clear}")
    
                local download_tool = download.new()
                download_tool:add_source_default()
                download_tool:fetch_manifests()
    
                for _, item in ipairs(lost_download) do
                    if item.opt.install_func == "tool" then
                        download_tool:download_tool(item.opt.name)
                    elseif item.opt.install_func == "sdk" then
                        download_tool:download_sdk(item.opt.name, {debug = item.opt.debug})
                    elseif item.opt.install_func == "resources" then
                        download_tool:download_file(item.opt.name)
                    elseif item.opt.install_func == "file" then
                        download_tool:download_file(item.opt.name)
                    else
                        raise("invalid install_func: %s", item.opt.install_func)
                    end
                end
            end
    
            -- install tools
            for _, item in ipairs(install_items) do
                if item.kind == "download" and item.opt.install_func == "tool" then
                    install.fill_opt(item.opt, item.trigger_target)
                    install.install_tool(item.opt)
                end
            end
        end
    end)
    on_clean(function (target)
        import("skr.utils")
        import("skr.analyze")

        -- cleanup analyze files
        analyze.clean_analyze_files()

        -- trigger analyze next time
        analyze.write_analyze_trigger_flag()

        -- remove config
        os.rm(utils.saved_config_path())
    end)
skr_global_target_end()

function skr_global_config()
    -- policy
    set_policy("build.ccache", false)
    set_policy("build.warning", true)
    set_policy("check.auto_ignore_flags", false)
    
    -- cxx options
    set_warnings("all")
    set_languages(get_config("cxx_version"), get_config("c_version"))
    add_rules("mode.debug", "mode.release", "mode.releasedbg", "mode.asan")

    -- cxx defines
    if (is_os("windows")) then 
        add_defines("UNICODE", "NOMINMAX", "_WINDOWS")
        add_defines("_GAMING_DESKTOP")
        add_defines("_CRT_SECURE_NO_WARNINGS")
        add_defines("_ENABLE_EXTENDED_ALIGNED_STORAGE")
        if (is_mode("release")) then
            set_runtimes("MD")
        elseif (is_mode("asan")) then
            add_defines("_DISABLE_VECTOR_ANNOTATION")
        else
            set_runtimes("MDd")
        end
    end
end 