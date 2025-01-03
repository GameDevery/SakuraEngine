executable_module("SketchUpDump", "SKETCHUP_DUMP_API", engine_version)
    set_group("04.examples/dcc/sketchup")
    public_dependency("SkrCore", engine_version)
    public_dependency("SkrTask", engine_version)
    public_dependency("SkrSketchUp", engine_version)
    set_exceptions("no-cxx")
    add_includedirs("include", {public = false})
    add_rules("c++.unity_build", {batchsize = default_unity_batch})
    add_files("src/**.cpp")

    -- install
    skr_install_rule()
    skr_install("files", {
        name = "sketchup-dump-skp",
        files = {"src/**.skp"},
        outdir = "/../resources/SketchUp",
        rootdir = "src"
    })