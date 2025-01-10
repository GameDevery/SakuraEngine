target("gamenetworkingsockets")
    set_group("00.frameworks")
    add_includedirs("include", {public = true})
    skr_install_rule()
    if is_os("windows") or is_os("macosx") then 
        skr_install("download", {
            name = "gns",
            install_func = "sdk",
        })
        set_kind("headeronly")
        add_links("gamenetworkingsockets", {public=true} )
    else
        print("error: gamenetworkingsockets is not built on this platform!")
    end