-- load saved data
function analyze_file_name(target)
    return path.join("build/.skr/analyze/", target:name() .. ".table")
end

-- load attributes set by skr_attributes()
function load_attributes(target)
    local attributes = target:values("Sakura.Attributes")
    if type(attributes) ~= "table" then
        attributes = { attributes }
    end
    return attributes
end

-- filter target to perform analyze
function filter_target(target, attributes)
    attributes = attributes or load_attributes(target)
    if attributes and attributes["Analyze.Ignore"] then
        return false
    end
    return true
end

-- load analyzer functions
function load_analyzers()
    local analyzer_target = project.target("Analyze.Phase")
    
    -- collect analyzer rules
    local analyzer_rules = {}
    for _, rule in ipairs(analyzer_target:orderules()) do
        if rule:name():startswith("__Analyzer.") then
            table.insert(analyzer_rules, rule)
        end
    end

    -- invoke analyzers
    local analyzers = {}
    for _, analyzer in ipairs(analyzer_rules) do
        local analyzer_name = analyzer:name():split("__Analyzer.")[1]
        local analyze_func = analyzer:script("build")
        analyzers[analyzer_name] = analyze_func
    end

    return analyzers
end

function load(target)
    local path = analyze_file_name(target)
    if os.exists(path) then
        return io.load(path)
    end
    return nil
end

function save(target, table)
    local path = analyze_file_name(target)
    io.save(path, table)
end
