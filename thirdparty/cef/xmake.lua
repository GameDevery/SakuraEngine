target("cef")
    set_kind("static")
    
    -- defines
    add_defines("WRAPPING_CEF_SHARED", {public=true})
    add_defines("NOMINMAX", {public=true})
    add_defines("USING_CEF_SHARED=1", {public=true})
    -- add_defines("BUILDING_CEF_SHARED=1", {private=true})

    -- include & src
    add_includedirs("include", {public=true})
    add_includedirs("./", {private=true})
    add_files("libcef_dll/**.cc")

    -- add links
    add_links("libcef")
    add_syslinks("User32")
    
    -- disable warnings
    add_cxxflags(
        "-Wno-undefined-var-template", 
        {tools={"gcc", "clang", "clang_cl"}}
    )

    -- install cef
    skr_install_rule()
    skr_install("download", {
        name = "cef-6778",
        install_func = "custom",
        custom_install = function (installer)
            installer.print_log = true
            installer:bin("Release/**")
            installer:bin("Resources/**", {
                rootdir = "Resources",
            })
        end
    })
