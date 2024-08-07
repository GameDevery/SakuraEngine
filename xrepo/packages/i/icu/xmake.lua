package("icu")
    set_homepage("https://github.com/unicode-org/icu/")
    set_description("International Components for Unicode.")
    set_license("ICU License. Details follow: https://github.com/unicode-org/icu/blob/main/LICENSE")
    
    add_versions("72.1.0-skr", "cd5f0085d268ebcae37eb0341987003481f32e6c0284c23398a187dbe9c836f6")

    add_deps("zlib =1.2.8-skr")
    if is_plat("windows") then
        add_syslinks("advapi32", {public = true})
    end

    on_install(function (package)
        os.mkdir(package:installdir())
        os.cp(path.join(package:scriptdir(), "port", "icu4c"), ".")
        os.cp(path.join(package:scriptdir(), "port", "xmake.lua"), "xmake.lua")

        local configs = {}
        import("package.tools.xmake").install(package, configs)
    end)
    
    on_test(function (package)
        assert(package:has_cfuncs("ucnv_convert", {includes = "unicode/ucnv.h"}))
    end)
