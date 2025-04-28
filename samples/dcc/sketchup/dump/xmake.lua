executable_module("SketchUpDump", "SKETCHUP_DUMP_API")
    set_group("04.examples/dcc/sketchup")
    public_dependency("SkrCore")
    public_dependency("SkrTask")
    public_dependency("SkrSketchUp")
    set_exceptions("no-cxx")
    add_includedirs("include", {public = false})
    skr_unity_build()
    add_files("src/**.cpp")

    -- install
    skr_install_rule()
    skr_install("files", {
        name = "sketchup-dump-skp",
        files = {"src/**.skp"},
        out_dir = "../resources/SketchUp",
        root_dir = "src"
    })