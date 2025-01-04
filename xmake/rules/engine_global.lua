function skr_global_target()
    target("Skr.Global")
end

function skr_global_target_end()
    target_end()
end


skr_global_target()
    set_kind("phony")
    set_policy("build.fence", true)
    on_load(function (target)
        import("skr.utils")
        import("skr.analyze")

        -- save config
        utils.save_config()

        -- trigger analyze
        if analyze.filter_analyze_trigger() then
            analyze.trigger_analyze()
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