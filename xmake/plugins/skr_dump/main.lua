import("core.base.option")
import("core.base.task")
import("core.base.global")
import("core.project.config")
import("core.project.project")
import("core.platform.platform")
import("core.base.json")
import("skr.utils")
import("skr.download")
import("skr.install")

function main()
    -- load config
    utils.load_config()
    -- load targets
    project.load_targets()

    -- dump env
    local global_target = project.target("Skr.Global")
    local env_value = global_target:values("skr.env")
    print("skr_env:")
    print(env_value)
end