-- imports
import("core.base.option")
import("core.base.task")
import("core.base.global")
import("core.project.config")
import("core.project.project")
import("core.platform.platform")
import("skr.analyze")
import("skr.utils")

function main()
    -- print("start analyze...")

    -- load config
    utils.load_config()
    
    -- load targets
    project.load_targets()

    -- load target info
    local targets_info = {}
    for _, target in pairs(project.ordertargets()) do
        -- load attributes
        local attributes = analyze.load_attributes(target)
        
        -- filter target
        if not analyze.filter_target(target, attributes) then
            goto continue
        end

        -- insert target info
        targets_info[target:name()] = {
            target = target,
            attributes = attributes
        }

        ::continue::
    end

    -- load analyzers
    local analyzers = analyze.load_analyzers()

    -- analyze and save tables
    local analyze_ctx = {
        query_attributes = function (target_name)
            return targets_info[target_name].attributes
        end,
        filter_target = function (target_name)
            return targets_info[target_name] ~= nil
        end,
    }
    for target_name, target_info in pairs(targets_info) do
        local table_for_save = {}
        for _, analyzer_info in ipairs(analyzers) do
            local analyzer_name = analyzer_info.name
            local analyzer = analyzer_info.func
            local result = analyzer(target_info.target, target_info.attributes, analyze_ctx)
            table_for_save[analyzer_name] = result
        end
        analyze.save(target_info.target:name(), table_for_save)
    end

    -- print("analyze ok!")
end