vk_includes_dir = path.join(engine_dir, "SDKs/vulkan")

target("vulkan")
    set_group("00.frameworks")
    set_kind("headeronly")
    add_includedirs(vk_includes_dir, {public=true})