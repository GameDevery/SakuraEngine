shared_module("SkrSketchUp", "SKR_SKETCH_UP")
    set_exceptions("no-cxx")
    add_deps("SkrBase", {public = true})
    public_dependency("SkrCore")
    add_includedirs("SketchUp", {public = true})
    add_files("src/build.*.cpp")
    add_linkdirs("$(buildir)/$(os)/$(arch)/$(mode)", {public=true})
    if is_os("windows") then 
        add_links("SketchUpAPI", {public = true})
    elseif is_os("macosx") then
        add_links(
            "CommonGeometry",
            "CommonGeoutils",
            "CommonImage",
            "CommonPreferences",
            "CommonUnits",
            "CommonUtils",
            "CommonZip"
        , {public = true})
    end

    -- install
    skr_install_rule()
    skr_install("download", {
        name = "sketchup-sdk-v2023.1.315",
        install_func = "sdk",
    })