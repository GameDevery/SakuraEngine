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

-- dump functions
local _dump_functions = {}
function _dump_functions.env()
    local global_target = project.target("Skr.Global")
    local env_value = global_target:values("skr.env")
    print(env_value)
end
function _dump_functions.modules_with_cull()
    local modules_with_cull = utils.get_env("modules_with_cull")
    modules_with_cull = table.unique(modules_with_cull)
    print(modules_with_cull)
end


function main()
    -- get options
    local opt_modules = option.get("modules")
    
    -- load config
    utils.load_config()
    -- load targets
    project.load_targets()

    if not opt_modules or #opt_modules == 0 then
        for k, v in pairs(_dump_functions) do
            cprint("${green}%s${clear}:", k)
            v()
        end
    else
        local not_found_modules = {}
        for _, module in ipairs(opt_modules) do
            if _dump_functions[module] then
                cprint("${green}%s:${clear}", module)
                _dump_functions[module]()
            else
                table.insert(not_found_modules, module)
            end
        end

        if #not_found_modules > 0 then
            cprint("${red}dump item not found: [%s]${clear}", table.concat(not_found_modules, ", "))
            cprint("can be:")
            for k, _ in pairs(_dump_functions) do
                cprint("  %s", k)
            end
        end
    end
end