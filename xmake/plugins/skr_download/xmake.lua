task("skr_download")
    set_category("plugin")
    on_run("main")
    
    set_menu {
            usage = "xmake skr_download [options] [packages]"
        ,   description = "download skr files from remote server"
        ,   options = {
            -- 缩写，选项，类型[k|kv|vs]，默认值，描述，选项...
                {'k', "kind",       "kv",   nil     , "download package kind"
                                                    , "   - tool"
                                                    , "   - resources"
                                                    , "   - sdk"
                                                    , "   - file"
                                                    }
            ,   {'f', "force",      "k",    nil     , "force download"}
            ,   {'d', "debug",      "k",    nil     , "download debug packages, only for sdk kind"}
            ,   {nil, "packages",   "vs",   nil     , "packages to download"}
        }
    }