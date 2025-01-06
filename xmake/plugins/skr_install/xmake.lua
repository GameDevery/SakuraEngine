task("skr_install")
    on_run("main")

    set_menu {
            usage = "xmake skr_install [options] [install items]"
        ,   description = "download skr files from remote server"
        ,   options = {
            -- 缩写，选项，类型[k|kv|vs]，默认值，描述，选项...
                {'k', "kind",       "kv",   nil     , "download package kind(install_func)"
                                                    , "   - tool"
                                                    , "   - resources"
                                                    , "   - sdk"
                                                    , "   - file"
                                                    }
            ,   {'m', "mode",       "kv",   nil     , "install kind"
                                                    , "   - download"
                                                    , "   - files"
                                                    }
            ,   {'l', "list",       "k",    nil     , "list all install items"}
            ,   {'f', "force",      "k",    nil     , "force install"}
            ,   {nil, "items",      "vs",   nil     , "items to install"}
        }
    }