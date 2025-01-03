shared_module("SkrGraphics", "SKR_GRAPHICS", engine_version)
    set_exceptions("no-cxx")
    add_deps("vulkan", "SkrBase", {public = true})
    public_dependency("SkrCore", engine_version)
    add_includedirs("include", {public = true})
    add_files("src/build.*.c", "src/build.*.cpp")
    if (is_os("macosx")) then 
        add_files("src/build.*.m", "src/build.*.mm")
        add_mxflags("-fno-objc-arc", {force = true})
        add_frameworks("CoreFoundation", "Cocoa", "Metal", "IOKit", {public = true})
    end
    
    if (is_os("windows")) then 
        add_linkdirs("$(buildir)/$(os)/$(arch)/$(mode)", {public=true})
        add_links("nvapi_x64", {public = true})
        add_links("WinPixEventRuntime", {public = true})
    end

    -- install
    skr_install_rule()
    skr_install("download", {
        name = "dstorage-1.2.2",
        install_func = "sdk",
        plat = { "windows" }
    })
    skr_install("download", {
        name = "dxc",
        install_func = "sdk",
        plat = { "windows" }
    })
    skr_install("download", {
        name = "amdags",
        install_func = "sdk",
        plat = { "windows" }
    })
    skr_install("download", {
        name = "nvapi",
        install_func = "sdk",
        plat = { "windows" }
    })
    skr_install("download", {
        name = "nsight",
        install_func = "sdk",
        plat = { "windows" }
    })
    skr_install("download", {
        name = "WinPixEventRuntime",
        install_func = "sdk",
        plat = { "windows" }
    })