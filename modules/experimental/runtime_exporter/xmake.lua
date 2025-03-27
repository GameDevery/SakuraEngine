codegen_component("SkrRTExporter", { api = "SKR_RUNTIME_EXPORTER", rootdir = "include/SkrRuntimeExporter" })
    add_files("include/**.h")
    add_files("include/**.hpp")

shared_module("SkrRTExporter", "SKR_RUNTIME_EXPORTER")
    public_dependency("SkrRT")
    skr_unity_build()
    on_load(function (target, opt)
        import("skr.utils")
        
        local includedir = path.join(os.scriptdir(), "include", "SkrRuntimeExporter", "exporters")
        local runtime = path.join(os.scriptdir(), "..", "..", "runtime", "include")
        -- cgpu/api.h
        local cgpu_header = path.join(runtime, "cgpu", "api.h")
        local cgpu_header2 = path.join(includedir, "cgpu", "api.h")
        
        -- copy cgpu headers
        utils.on_changed(function (change_info)
            os.vcp(cgpu_header, cgpu_header2)
        end,{
            cache_file = utils.depend_file_target(target:name(), cgpu_header),
            files = {cgpu_header, cgpu_header2, target:targetfile()},
            use_sha = true,
        }
    )
        target:add("defines",
            "CGPU_EXTERN_C_BEGIN= ", "CGPU_EXTERN_C_END= ",
            "CGPU_EXTERN_C= ", "CGPU_API=sreflect")
    end)
    add_files("src/**.cpp")