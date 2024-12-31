-----------------------dir & file-----------------------
-- load saved data
function analyze_file_name(target)
    return path.join("build/.skr/analyze/", target:name() .. ".table")
end

-----------------------attributes-----------------------
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

-----------------------analyzers-----------------------
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

-----------------------analyzer archive-----------------------
_analyze_cache = _analyze_cache or {}
function load(target)
    -- load from cache
    local exist_analyze = _analyze_cache[target:name()]
    if exist_analyze then
        return exist_analyze
    end

    -- load from file
    local path = analyze_file_name(target)
    if os.exists(path) then
        local loaded_analyze = io.load(path)
        if loaded_analyze then
            _analyze_cache[target:name()] = loaded_analyze
            return loaded_analyze
        end
    end
    return nil
end
function save(target, table)
    -- save to cache
    _analyze_cache[target:name()] = table

    -- save analyze
    io.save(analyze_file_name(target), table)
end
function clear_cache()
    _analyze_cache = {}
end
function clear_cache_for(target)
    _analyze_cache[target:name()] = nil
end
