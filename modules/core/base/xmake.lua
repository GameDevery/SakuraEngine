static_component("SkrBase", "SkrCore")
    skr_unity_build()
    add_deps("SkrProfile", {public = true})
    add_deps("SkrCompileFlags", {public = true})
    add_includedirs("include", {public = true})
    add_files("src/**/build.*.c", "src/**/build.*.cpp")
    -- for guid/uuid
    if (is_os("windows")) then 
        add_syslinks("Ole32", {public = true})
    end
    if (is_os("macosx")) then 
        add_frameworks("CoreFoundation", {public = true})
    end

    -- natvis
    skr_dbg_natvis_files("dbg/**.natvis")