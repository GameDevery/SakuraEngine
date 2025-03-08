target("mimalloc")
    set_group("01.libraries")
    set_optimize("fastest")
    add_files("files/build.mimalloc.cpp")
    add_includedirs("files", {public = true})
    if (is_os("windows")) then 
        add_syslinks("psapi", "shell32", "user32", "advapi32", "bcrypt")
        add_defines("_CRT_SECURE_NO_WARNINGS")
    end

    set_kind("shared")
    add_defines("MI_SHARED_LIB", {public = true})
    add_defines("MI_SHARED_LIB_EXPORT", "MI_XMALLOC=1", "MI_WIN_NOREDIRECT")