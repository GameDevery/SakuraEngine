codegen_component("SkrRTExporter", { api = "SKR_RUNTIME_EXPORTER", rootdir = "include/SkrRuntimeExporter" })
    add_files("include/**.h")
    add_files("include/**.hpp")

shared_module("SkrRTExporter", "SKR_RUNTIME_EXPORTER")
    public_dependency("SkrRT")
    add_rules("c++.unity_build", {batchsize = default_unity_batch})
    on_load(function (target, opt)
        import("skr.utils")
        
        local includedir = path.join(os.scriptdir(), "include", "SkrRuntimeExporter", "exporters")
        local runtime = path.join(os.scriptdir(), "..", "..", "runtime", "include")
        -- cgpu/api.h
        local cgpu_header = path.join(runtime, "cgpu", "api.h")
        local cgpu_header2 = path.join(includedir, "cgpu", "api.h")
        if utils.is_changed({
            cache_file = target:dependfile(cgpu_header),
            files = {cgpu_header, cgpu_header2, target:targetfile()},
        }) then
            os.vcp(cgpu_header, cgpu_header2)
        end
        target:add("defines",
            "CGPU_EXTERN_C_BEGIN= ", "CGPU_EXTERN_C_END= ",
            "CGPU_EXTERN_C= ", "CGPU_API=sreflect")
    end)
    add_files("src/**.cpp")