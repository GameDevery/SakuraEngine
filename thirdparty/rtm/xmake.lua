target("rtm")
    set_kind("headeronly")

    -- add packages
    add_includedirs("include", {public = true})
    add_files("rtm_dummy_src_for_clangd.cpp")
    skr_dbg_natvis_files("rtm.natvis")
