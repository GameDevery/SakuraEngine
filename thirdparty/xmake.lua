includes("gamenetworkingsockets/xmake.lua")
includes("mimalloc/xmake.lua")
includes("SDL2/xmake.lua")
includes(("vulkan/xmake.lua"))

skr_includes_with_cull("v8", function ()
    includes('v8')
end)