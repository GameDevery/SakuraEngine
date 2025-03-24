-- analyzer_target("DisableSingle")
--     analyze(function(target, attributes, analyzing)
--         import("skr.utils")

--         -- global info
--         local _engine_dir = utils.get_env("engine_dir")

--         local scriptdir = path.relative(target:scriptdir(), _engine_dir):gsub("\\", "/")

--         local is_core = scriptdir:startswith("modules/core")
--         local is_render = scriptdir:startswith("modules/render")
--         local is_devtime = scriptdir:startswith("modules/devtime")
--         local is_gui = scriptdir:startswith("modules/gui")
--         local is_dcc = scriptdir:startswith("modules/dcc")
--         local is_tools = scriptdir:startswith("modules/tools")
--         local is_experimental = scriptdir:startswith("modules/experimental")

--         -- TODO: move v8 out of engine
--         local is_v8 = scriptdir:startswith("modules/engine/v8")
--         local is_engine = scriptdir:startswith("modules/engine") and not is_v8

--         local is_sample = scriptdir:startswith("samples")
--         local is_tests = scriptdir:startswith("tests")

--         local is_thirdparty = 
--             not scriptdir:startswith("modules") and
--             not scriptdir:startswith("samples") and
--             not scriptdir:startswith("editors") and
--             not scriptdir:startswith("tools") and
--             not scriptdir:startswith("tests")

--         local enable = is_thirdparty or is_core or is_engine or is_render or is_gui or is_dcc or is_tests or is_sample
--         target:data_set("_Disable", not enable)
--         return not enable
--     end)
-- analyzer_target_end()

-- analyzer_target("Disable")
--     add_deps("__Analyzer.DisableSingle", { order = true })
--     analyze(function(target, attributes, analyze_ctx)
--         local _disable = target:data("_Disable") or false
--         if not _disable then
--             for _, dep in pairs(target:deps()) do
--                 if dep:data("_Disable") then
--                     _disable = true
--                     break
--                 end
--             end
--         end
--         return _disable
--     end)
-- analyzer_target_end()

-- skr_cull("cef")
-- skr_cull("v8")
skr_cull("lua")