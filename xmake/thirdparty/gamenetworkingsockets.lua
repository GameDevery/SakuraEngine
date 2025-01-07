gns_target_dir = path.join(engine_dir, "thirdparty/gamenetworkingsockets")
gns_include_dir = path.join(engine_dir, "thirdparty/gamenetworkingsockets/include")

target("gamenetworkingsockets")
    set_group("00.frameworks")
    add_includedirs(gns_include_dir, {public = true})
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