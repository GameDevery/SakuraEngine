target("cef_simple")
    set_kind("binary")
    add_deps("cef")
    add_files("src/**.cc")