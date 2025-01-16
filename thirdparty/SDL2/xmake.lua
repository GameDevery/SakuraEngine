target("SDL2")
    set_kind("headeronly")

    -- add packages
    add_includedirs("include", {public = true})
    
    -- download prebuilt binaries
    skr_install_rule()
    skr_install("download", {
        name = "SDL2",
        install_func = "sdk",
    })

    -- add links
    add_links("SDL2", {public = true})