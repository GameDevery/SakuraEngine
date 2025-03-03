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