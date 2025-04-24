target("rtm")
    set_kind("headeronly")

    -- add packages
    add_includedirs("include", {public = true})
    skr_dbg_natvis_files("rtm.natvis")
