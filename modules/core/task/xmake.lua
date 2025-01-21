-- add_requires("boost-context >=0.1.0-skr")
-- TODO. remove boost_context and FiberTaskingLib backend

shared_module("SkrTask", "SKR_TASK")
    -- add source files
    public_dependency("SkrCore")
    add_includedirs("include", {public = true})
    add_files("src/build.*.cpp")
    -- internal packages
    -- add_packages("boost-context", {public = true, inherit = true})
    -- add FTL source 
    -- local ftl_includes_dir = path.join(skr_engine_dir, "thirdparty/FiberTaskingLib/include")
    -- add_includedirs(ftl_includes_dir, {public = true})
    -- add_files(path.join(skr_engine_dir, "thirdparty/FiberTaskingLib/source/build.*.cpp"))
    -- add marl source
    local marl_source_dir = path.join(skr_engine_dir, "thirdparty/marl")
    add_files(marl_source_dir.."/src/build.*.cpp")
    if not is_os("windows") then 
        add_files(marl_source_dir.."/src/**.c")
        add_files(marl_source_dir.."/src/**.S")
    end
    add_includedirs(path.join(skr_engine_dir, "thirdparty/marl/include"), {public = true})
    -- marl runtime compile definitions
    after_load(function (target,  opt)
        if (target:get("kind") == "shared") then
            import("core.project.project")
            target:add("defines", "MARL_DLL", {public = true})
            target:add("defines", "MARL_BUILDING_DLL")
        end
    end)

--[[
shared_pch("SkrTask")
    add_files("include/SkrTask/**.hpp")
    add_files(path.join(skr_engine_dir, "modules/core/base/include/**.h"))
    add_files(path.join(skr_engine_dir, "modules/core/base/include/**.hpp"))
    add_files(path.join(skr_engine_dir, "modules/core/core/include/**.h"))
    add_files(path.join(skr_engine_dir, "modules/core/core/include/**.hpp"))
]]--