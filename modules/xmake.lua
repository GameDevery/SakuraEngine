includes("core/xmake.lua")

skr_includes_with_cull("engine", function ()
    includes("engine/xmake.lua")
end)

skr_includes_with_cull("render", function ()
    includes("render")
end)

skr_includes_with_cull("devtime", function ()
    includes("devtime")
end)

skr_includes_with_cull("gui", function ()
    includes("gui")
end)

skr_includes_with_cull("dcc", function ()
    includes("dcc")
end)

skr_includes_with_cull("tools", function ()
    includes("tools")
end)

skr_includes_with_cull("experimental", function ()
    -- includes("experimental/netcode/xmake.lua")
    -- includes("experimental/physics/xmake.lua")
    -- includes("experimental/tweak/xmake.lua")
    -- includes("experimental/inspector/xmake.lua")
    -- includes("experimental/runtime_exporter/xmake.lua")
    -- includes("experimental/daScript/xmake.lua")
end)