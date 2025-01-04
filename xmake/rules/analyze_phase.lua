-- target to trigger analyze when configure phase
target("Analyze.Phase")
    set_kind("phony")
    set_policy("build.fence", true)
    on_load(function(phase)
        import("skr.utils")
        import("skr.analyze")
        
        if analyze.filter_analyze_trigger() then
            utils.save_config()
            analyze.trigger_analyze()
        end
    end)
    on_clean(function(phase)
        import("skr.analyze")
        analyze.write_analyze_trigger_flag()
    end)
target_end()

-- analyze tool functions
function analyzer_target(name)
    target("Analyze.Phase")
        add_rules("__Analyzer."..name)
        set_default(false)
    target_end()
    rule("__Analyzer."..name)
end
function analyze(func, opt)
    on_build(func, opt)
end
function analyzer_target_end()
    rule_end()
end
function analyzer_attribute(attribute)
    add_values("Sakura.Attributes", attribute)
end
function analyzer_ignore()
    analyzer_attribute("Analyze.Ignore")
end

-- solve dependencies
analyzer_target("Dependencies")
    analyze(function(target, attributes, analyze_ctx)
        local dependencies = {}
        local idx = 1
        for __, dep in ipairs(target:orderdeps()) do
            local dep_name = dep:name()
            if analyze_ctx.filter_target(dep_name) then
                dependencies[idx] = dep_name
                idx = idx + 1
            end
        end
        return dependencies
    end)
analyzer_target_end()

-- solve attributes
analyzer_target("Attributes")
    analyze(function(target, attributes, analyzing)
        return attributes
    end)
analyzer_target_end()