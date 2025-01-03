target("OpenGUI_DemoResources")
    set_kind("static")
    set_group("04.examples/application")
    add_includedirs("common", {public = true})
    add_includedirs("./../../common", {public = true})
    add_includedirs("common", {public = true})
    add_files("common/**.cpp")
    public_dependency("SkrInput", engine_version)
    public_dependency("SkrGui", engine_version)
    public_dependency("SkrGuiRenderer", engine_version)

    -- install
    skr_install_rule()
    skr_install("files", {
        name = "ogui-demo-resources",
        files = {"common/**.png"},
        outdir = "/../resources/OpenGUI",
        rootdir = "common"
    })

-- includes("gdi/xmake.lua")
-- includes("robjects/xmake.lua")
includes("sandbox/xmake.lua")