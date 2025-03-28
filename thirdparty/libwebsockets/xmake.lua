target("libwebsockets")
    set_kind("headeronly")

    -- add packages
    add_includedirs("include", {public = true})

    -- download prebuilt binaries
    skr_install_rule()
    skr_install("download", {
        name = "libwebsockets",
        install_func = "sdk",
    })

    -- add links
    add_links("websockets_static", {public=true})
    add_syslinks("ws2_32", {public=true})