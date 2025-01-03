rule("DisableTargets")
    on_load(function(target)
        import("skr.analyze")

        if xmake.argv()[1] ~= "analyze_project" then
            local analyze_tbl = analyze.load(target)
            if analyze_tbl then
                local disable = analyze_tbl["Disable"]
                if disable then
                    target:set("default", false)
                else
                    local _default = target:get("default")
                    local _ = (_default == nil) or (_default == true)
                    if (_default == nil) or (_default == true) then
                        target:set("default", true)
                    end
                end
            end
        end
    end)
rule_end()