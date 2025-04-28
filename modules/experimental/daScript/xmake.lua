add_requires("daScript 2023.4.26-skr.1")

includes("rules/AOT.lua")

target("daScriptVSCode")
    set_group("06.devs")
    set_kind("phony")
    add_packages("daScript", { public = false })
    add_rules("@daScript/Standard", {
        outdir = path.absolute(path.join(skr_env_get("engine_dir"), "/.vscode")) 
    })

shared_module("SkrDAScript", "SKR_DASCRIPT")
    add_deps("daScriptVSCode")
    set_exceptions("no-cxx")
    set_optimize("fastest")
    add_packages("daScript", { public = false })
    add_rules("@daScript/Standard", { outdir = "." })
    add_includedirs("daScript/include", { public=true })
    add_files("daScript/src/**.cpp")
    skr_unity_build()
    public_dependency("SkrRTExporter")