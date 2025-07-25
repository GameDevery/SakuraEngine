add_requires("freetype >=2.13.0-skr", {system = false})
add_requires("icu >=72.1.0-skr", {system = false})
add_requires("harfbuzz >=7.1.0-skr", {system = false})

add_requires("nanovg >=0.1.0-skr", {system = false})

codegen_component("SkrGui", { api = "SKR_GUI", rootdir = "include/SkrGui" })
    add_files("include/**.hpp")

shared_module("SkrGui", "SKR_GUI")
    add_packages("freetype", "icu", "harfbuzz")
    add_packages("nanovg")
    public_dependency("SkrRT")

    -- unity build & pch
    skr_unity_build()
    add_includedirs("include", {public = true})
    add_includedirs("src", {public = false})
    add_files("src/*.cpp")
    add_files("src/math/**.cpp")
    add_files("src/framework/**.cpp", {unity_group = "framework"})
    add_files("src/render_objects/**.cpp", {unity_group = "render_objects"})
    add_files("src/widgets/**.cpp", {unity_group = "widgets"})
    add_files("src/dev/**.cpp", {unity_group = "dev"})
    add_files("src/system/**.cpp", {unity_group = "system"})

    add_files("src/backend/*.cpp")
    add_files("src/backend/paragraph/*.cpp", {unity_group  = "text"})
    add_files("src/backend/text_server/*.cpp", {unity_group  = "text"})
    add_files("src/backend/text_server_adv/*.cpp", {unity_group  = "text_adv"})
    
    remove_files("src/dev/deprecated/**.cpp")

private_pch("SkrGui")
    add_deps("SkrGui.Mako")
    add_files("include/**.hpp")