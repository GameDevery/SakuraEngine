target("AppSampleCommon")
    set_kind("headeronly")
    add_includedirs("include", {public = true})
    add_deps("SDL3", {public = true})

    -- for clangd
    add_files("_app_sample_dummy_src.cpp")