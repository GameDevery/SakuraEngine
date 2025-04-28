target("libhv")
    set_kind("headeronly")

    add_defines("HV_STATICLIB", {public=true})

    -- add includes
    add_includedirs("include", {public = true})

    -- add links
    add_links("hv_static", {public=true})
    -- add_syslinks("ws2_32", {public=true})

    -- download prebuilt binaries
    skr_install_rule()
    skr_install("download", {
        name = "libhv",
        install_func = "sdk",
    })
