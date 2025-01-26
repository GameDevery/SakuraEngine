task("skr_dump")
    set_category("plugin")
    on_run("main")

    set_menu {
            usage = "xmake skr_dump [options] [dump modules]"
        ,   description = "download skr files from remote server"
        ,   options = {
            -- 缩写，选项，类型[k|kv|vs]，默认值，描述，选项...
                {nil, "modules",      "vs",   nil     , "modules to install"}
        }
    }