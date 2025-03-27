add_requires("zlib >=1.2.8-skr", {system = false})

shared_module("SkrImageCoder", "SKR_IMAGE_CODER")
    public_dependency("SkrRT")
    add_includedirs("include", {public=true})
    add_files("src/**.cpp")
    skr_unity_build()
    add_packages("zlib")

    -- jpeg
    add_includedirs("include", "turbojpeg", {public=true})
    
    -- add include
    add_includedirs("include", {public=true})
    add_includedirs("libpng/1.5.2", {public=true})

    -- add link
    if is_os("windows") then
        add_links("libpng15_static", {public=true})
        add_links("turbojpeg_static", {public=true})
    elseif is_os("macosx") then
        add_links("png", {public=true})
        add_links("turbojpeg", {public=true})
    end

    -- add install
    skr_install_rule()
    skr_install("download", {
        name = "libpng",
        install_func = "sdk",
    })
    skr_install("download", {
        name = "turbojpeg",
        install_func = "sdk"
    })