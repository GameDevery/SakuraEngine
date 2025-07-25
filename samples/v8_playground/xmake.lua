codegen_component("V8Playground", { api = "V8_PLAYGROUND", rootdir = "include/V8Playground" })
    add_files("include/**.hpp")

executable_module("V8Playground", "V8_PLAYGROUND")
    public_dependency("SkrV8")
    add_files("src/**.cpp")
    add_includedirs("include", {public = true})

    -- install
    skr_install_rule()
    -- skr_install("files", {
    --     name = "v8-playground-scripts",
    --     files = {
    --         "script/**.js"
    --     },
    --     out_dir = "../script",
    --     root_dir = "script",
    -- })

    -- dbg
    skr_dbg_args(path.join(os.scriptdir(), "script/output/main.js"))
    skr_dbg_cmd_pre("xmake run V8Playground d -o "..path.join(os.scriptdir(), "script/types"))
    skr_dbg_cmd_pre("tsc -p "..path.join(os.scriptdir(), "script/tsconfig.json"))