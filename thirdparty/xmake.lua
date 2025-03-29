includes("gamenetworkingsockets/xmake.lua")
includes("libwebsockets")
includes("mimalloc/xmake.lua")
includes("SDL2/xmake.lua")
includes("vulkan/xmake.lua")
includes("libhv")

skr_includes_with_cull("v8", function ()
    includes('v8')
end)

skr_includes_with_cull("cef", function ()
    includes('cef')
end)