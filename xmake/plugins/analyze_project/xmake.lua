task("analyze_project")
    set_category("plugin")
    on_run("main")
    
    set_menu {
        usage = "xmake analyze_project [options]",
        description = "run tests",
        options = {}
    }
