if is_os("windows") or is_os("macosx") or is_os("linux") then
    if(not has_config("shipping_one_archive")) then
        add_requires("efsw")
        efsw_pak = true
    end
end

shared_module("SkrTweak", "SKR_TWEAK")
    public_dependency("SkrRT")
    add_includedirs("include", {public=true})
    skr_unity_build()
    add_files("src/**.cpp")
    add_defines(format("SKR_SOURCE_ROOT=R\"(%s)\"", skr_env_get("engine_dir")), {public=false})
    if(efsw_pak) then
        add_packages("efsw")
    end
