includes("graphics/xmake.lua")
includes("image_coder/xmake.lua")
includes("runtime/xmake.lua")
includes("scene/xmake.lua")
includes("input/xmake.lua")
includes("input_system/xmake.lua")

skr_includes_with_cull("v8", function ()
    includes("v8/xmake.lua")
end)

skr_includes_with_cull("lua", function ()
    includes("lua/xmake.lua")
end)