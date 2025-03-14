import("core.base.option")
import("core.base.json")
import("core.project.project")
import("core.language.language")
import("core.tool.compiler")
import("skr.utils")

-- TODO. move depend files into .skr
-- TODO. cleanup codegen artifacts

-- programs
local _meta = utils.find_meta()
-- local _python = utils.find_python()
local _bun = utils.find_bun()

-- data cache names
local _codegen_data_batch_name = "c++.codegen.batch"
local _codegen_data_headers_name = "c++.codegen.headers"
local _codegen_data_generators_name = "c++.codegen.generators"

-- rule names
local _codegen_rule_generators_name = "c++.codegen.generators"

-- global info
local _engine_dir = utils.get_env("engine_dir")

-- macos get include dirs
local macos_include_dirs = {}
if is_plat("macosx") then
    local out, err = os.iorunv("clang", {"-x", "c++", "-v", "-E", path.join(_engine_dir, "tools/null.cpp")})
    do
        local start_str = "#include <...> search starts here:"
        local end_str = "End of search list."
        local start_begin, start_end = err:find(start_str, 1, true)
        local end_begin, end_end = err:find(end_str, 1, true)
        local include_str = err:sub(start_end + 1, end_begin - 1)
        local includes = include_str:split("\n")
        for _, include in ipairs(includes) do
            include = string.trim(include)
            if string.endswith(include, "(framework directory)") then
                include = string.trim(include:sub(1, #include - #"(framework directory)"))
            end
            if include ~= "" then
                table.insert(macos_include_dirs, include)
            end
        end
    end
end
-- print(macos_include_dirs)

-- steps
--  1. collect header batch
--  2. solve generators
--  3. compile headers to meta json
--  4. call codegen scripts
function collect_headers_batch(target, headerfiles)
    -- get config
    local batchsize = 999 -- batch size optimization is disabled now
    local sourcedir = path.join(utils.skr_codegen_dir(target:name()), "meta_src")
    local metadir = path.join(utils.skr_codegen_dir(target:name()), "meta_database")
    local gendir = path.join(utils.skr_codegen_dir(target:name()), "codegen", target:name())

    -- build batches
    local sourcefile = path.join(sourcedir, "reflection_batch" .. ".cpp")
    local meta_batch = {
        sourcefile = sourcefile,
        metadir = metadir,
        gendir = gendir,
        headerfiles = {}
    }
    local file_count = 0
    for _, headerfile in pairs(headerfiles) do
        -- add source to batch
        table.insert(meta_batch.headerfiles, headerfile)
        file_count = file_count + 1
    end

    -- set data
    target:data_set(_codegen_data_batch_name, meta_batch)
    target:data_set(_codegen_data_headers_name, headerfiles)
end

function solve_generators(target)
    -- get config
    local generator_config = target:extraconf("rules", _codegen_rule_generators_name)
    
    -- solve config
    local solved_config = {
        scripts = {},
        dep_files = {}
    }

    -- solve scripts
    for _, script_config in ipairs(generator_config.scripts) do
        -- collect import dirs
        local import_dirs = {}
        if script_config.import_dirs then
            for _, dir in ipairs(script_config.import_dirs) do
                table.insert(import_dirs, path.join(target:scriptdir(), dir))
            end 
        end
        
        -- save config
        table.insert(solved_config.scripts, {
            file = path.join(target:scriptdir(), script_config.file),
            private = script_config.private,
            import_dirs = import_dirs,
        })
    end

    -- solve dep files
    for _, dep_file in ipairs(generator_config.dep_files) do
        local match_files = os.files(path.join(target:scriptdir(), dep_file))
        for __, file in ipairs(match_files) do
            table.insert(solved_config.dep_files, file)
        end
    end

    -- save config
    target:data_set(_codegen_data_generators_name, solved_config)

    -- permission
    if (os.host() == "macosx") then
        os.exec("chmod 777 ".._meta)
    end
end

function _codegen_compile(target, proxy_target, opt)
    local headerfiles = target:data(_codegen_data_headers_name)
    local batchinfo = target:data(_codegen_data_batch_name)
    local sourcefile = batchinfo.sourcefile
    local outdir = batchinfo.metadir
    local rootdir = path.absolute(proxy_target:data("meta.rootdir")) or path.absolute(target:scriptdir())

    opt = opt or {}
    opt.target = target
    opt.rawargs = true

    local meta_std_dir = path.join(utils.install_dir_tool(), "meta-include")
    local start_time = os.time()
    local using_msvc = target:toolchain("msvc")
    local using_clang_cl = target:toolchain("clang-cl")

    -- fix macosx include solve bug
    if is_plat("macosx") then
        for _, dep_target in pairs(target:orderdeps()) do
            local dirs = dep_target:get("includedirs")
            target:add("includedirs", dirs)
        end
    end

    -- load compiler and get compilation command
    local sourcekind = opt.sourcekind
    if not sourcekind and type(sourcefile) == "string" then
        sourcekind = language.sourcekind_of(sourcefile)
    end
    local compiler_inst = compiler.load(sourcekind, opt)
    local program, argv = compiler_inst:compargv(sourcefile, sourcefile..".o", opt)
    
    -- remove source args
    do
        local new_arg = {}
        local obj_flag =  "-Fo" .. sourcefile .. ".o"
        for i, arg in pairs(argv) do
            if arg ~= sourcefile and arg ~= obj_flag then
                table.insert(new_arg, arg)
            end
        end
        argv = new_arg
    end
    
    -- remove pch flags
    if using_msvc or using_clang_cl then
        for k, arg in pairs(argv) do 
            if arg:startswith("-Yu") or arg:startswith("-FI") or arg:startswith("-Fp") then
                argv[k] = nil 
            end
        end
    else
        for i, arg in pairs(argv) do
            if arg:find("_pch.hpp") or arg:find("_pch.pch") then
                argv[i - 1] = nil
                argv[i] = nil
            end
        end
    end
    do
        local new_argv = {}
        for _, arg in pairs(argv) do
            if arg then
                table.insert(new_argv, arg)
            end
        end
        argv = new_argv
    end

    -- setup tool flags
    if using_msvc or using_clang_cl then
        table.insert(argv, "--driver-mode=cl")
    end

    -- setup std lib include
    if os.host() == "windows" then
        table.insert(argv, "-I"..path.join(utils.install_dir_tool(), "meta-include"))
    elseif os.host() == "macosx" then
        for _, dir in ipairs(macos_include_dirs) do
            table.insert(argv, "-isystem")
            table.insert(argv, dir)
        end
    end

    -- setup disable warning flag
    table.insert(argv, "-Wno-abstract-final-class")

    -- hack: insert a placeholder to avoid the case where (#argv < limit) and (#argv + #argv2 > limit)
    local argv2 = {
        sourcefile, 
        "--output="..path.absolute(outdir), 
        "--root="..rootdir,
        "--"
    }
    if is_host("windows") then
        local argv2_len = 0
        argv2_len = 1024 * 8
        for _, v in pairs(argv2) do
            argv2_len = argv2_len + #v
        end
        local hack_opt = "-DFUCK_YOU_WINDOWS" .. string.rep("S", argv2_len)
        table.insert(argv, #argv + 1, hack_opt)
        argv = winos.cmdargv(argv)
    end
    for k,v in pairs(argv2) do  
        table.insert(argv, k, v)
    end
    
    -- print commands
    local command = _meta .. " " .. table.concat(argv, " ")
    if option.get("verbose") then
        cprint(
            "${green}[%s]: compiling.meta ${clear}%ss\n%s"
            , target:name()
            , path.relative(outdir)
            , command
        )
    elseif not opt.quiet then
        cprint(
            "${green}[%s]: compiling.meta ${clear}%ss"
            , target:name()
            , path.relative(outdir)
            -- , command
        )
    end

    -- compile meta source
    local out, err = os.iorunv(_meta, argv)
    
    -- dump output
    if option.get("verbose") and out and #out > 0 then
        print("=====================["..target:name().." meta output]=====================")
        printf(out)
        print("=====================["..target:name().." meta output]=====================")
    end
    if err and #err > 0 then
        print("=====================["..target:name().." meta error]=====================")
        printf(err)
        print("=====================["..target:name().." meta error]=====================")
    end

    if not opt.quiet then
        cprint(
            "${green}[%s]: finish.meta ${clear}%s cost ${red}%d seconds"
            , target:name()
            , path.relative(outdir)
            , os.time() - start_time
        )
    end
end

function meta_compile(target, proxy_target, opt)
    local headerfiles = target:data(_codegen_data_headers_name)
    local batchinfo = target:data(_codegen_data_batch_name)
    local sourcefile = batchinfo.sourcefile
    
    -- collect depend files
    local depend_files = { _meta }
    for _, headerfile in ipairs(headerfiles) do
        table.insert(depend_files, headerfile)
    end

    -- generate headers dummy
    if(headerfiles ~= nil and #headerfiles > 0) then
        utils.on_changed(function (change_info)
            local verbose = option.get("verbose")
            sourcefile = path.absolute(sourcefile)
            
            -- generate unity source
            local unity_cpp = io.open(sourcefile, "w")
            local content_string = ""
            for _, headerfile in ipairs(headerfiles) do
                headerfile = path.absolute(headerfile)
                local relative_include = path.relative(headerfile, path.directory(sourcefile))
                relative_include = relative_include:gsub("\\", "/")
                content_string = content_string .. "#include \"" .. relative_include .. "\"\n"
                if verbose then
                    cprint("${magenta}[%s]: meta.header ${clear}%s", target:name(), path.relative(headerfile))
                end
            end
            unity_cpp:print(content_string)
            unity_cpp:close()

            -- build generated cpp to json
            _codegen_compile(target, proxy_target, opt)
        end, {
            cache_file = utils.depend_file(path.join("codegen", target:name(), "codegen.meta")),
            files = depend_files,
            use_sha = true,
            post_scan = function ()
                local batchinfo = target:data(_codegen_data_batch_name)
                local outdir = batchinfo.metadir
                return os.files(path.join(outdir, "**.meta"))
            end
        })
    end
end

----------------------------------------------------------------------------------------------------------------------------------------------

function _mako_render(target, scripts, dep_files, opt)
    -- get config
    local batchinfo = target:data(_codegen_data_batch_name)
    local headerfiles = target:data(_codegen_data_headers_name)
    local sourcefile = batchinfo.sourcefile
    local metadir = batchinfo.metadir
    local gendir = batchinfo.gendir

    local api = target:values("c++.codegen.api")
    local generate_script = path.join(_engine_dir, "/tools/meta_codegen_ts/codegen.ts")
    -- local generate_script = path.join(_engine_dir, "/tools/meta_codegen/codegen.py")
    local start_time = os.time()

    if not opt.quiet then
        cprint("${cyan}[%s]: %s${clear} %s", target:name(), path.filename(generate_script), path.relative(metadir))
    end

    -- config
    local config = {
        output_dir = path.absolute(gendir),
        main_module = {
            module_name = target:name(),
            meta_dir = path.absolute(metadir),
            api = api and api:upper() or target:name():upper(),
        },
    }

    -- collect include modules
    config.include_modules = {}
    for _, dep_target in pairs(target:deps()) do
        local dep_api = dep_target:values("c++.codegen.api")
        table.insert(config.include_modules, {
            module_name = dep_target:name(),
            meta_dir = path.absolute(path.join(utils.skr_codegen_dir(dep_target:name()), "meta_database")),
            api = dep_api and dep_api:upper() or dep_target:name():upper(),
        })
    end

    -- collect generators
    config.generators = {}
    for _, script in pairs(scripts) do
        table.insert(config.generators, {
            entry_file = script.file,
            import_dirs = script.import_dirs,
        })
    end

    -- output config
    config_file = path.join(utils.skr_codegen_dir(target:name()), "codegen", target:name().."_codegen_config.json")
    json.savefile(config_file, config)

    -- baisc commands
    local command = {
        generate_script,
        config_file
    }

    if verbos then
        cprint(
            "[%s] bun %s"
            , target:name()
            , table.concat(command, " ")
        )
    end

    -- call codegen script
    -- local out, err = os.iorunv(_python, command)
    local out, err = os.iorunv(_bun, command)

    
    -- dump output
    if option.get("verbose") and out and #out > 0 then
        print("=====================["..target:name().." mako output]=====================")
        printf(out)
        print("=====================["..target:name().." mako output]=====================")
    end
    if err and #err > 0 then
        print("=====================["..target:name().." mako error]=====================")
        printf(err)
        print("=====================["..target:name().." mako error]=====================")
    end

    if not opt.quiet then
        cprint(
            "${cyan}[%s]: %s${clear} %s cost ${red}%d seconds"
            , target:name()
            , path.filename(generate_script)
            , path.relative(metadir)
            , os.time() - start_time)
    end
end

function mako_render(target, opt)
    -- get config
    local batchinfo = target:data(_codegen_data_batch_name)
    local metadir = batchinfo.metadir

    -- collect framework depend files
    local dep_files = os.files(path.join(metadir, "**.meta"))
    do
        -- local framework_patterns = {
        --     path.join(_engine_dir, "tools/meta_codegen/**.py"),
        --     path.join(_engine_dir, "tools/meta_codegen/**.mako"),
        -- }
        local framework_patterns = {
            path.join(_engine_dir, "tools/meta_codegen_ts/framework/**"),
            path.join(_engine_dir, "tools/meta_codegen_ts/codegen.ts"),
        }
        for _, pattern in ipairs(framework_patterns) do
            for _, file in ipairs(os.files(pattern)) do
                table.insert(dep_files, file)
            end
        end
    end

    -- collect target depend files
    -- TODO. use config comapre
    table.insert(dep_files, path.join(target:scriptdir(), "xmake.lua"))
    for _, dep_target in ipairs(target:deps()) do
        target.insert(dep_files, path.join(dep_target:scriptdir(), "xmake.lua"))
    end
    
    local scripts = {}

    -- extract self generator scripts and dep_files
    if target:data(_codegen_data_generators_name) then
        local self_generator_config = target:data(_codegen_data_generators_name)
        for _, script_config in ipairs(self_generator_config.scripts) do
            table.insert(scripts, script_config)
        end
        for _, dep_file in ipairs(self_generator_config.dep_files) do
            table.insert(dep_files, dep_file)
        end
    end

    -- extract dep generator scripts and dep_files
    for _, dep in pairs(target:deps()) do
        local dep_generator = dep:data(_codegen_data_generators_name)
        if dep_generator then
            -- extract scripts
            for __, script_config in ipairs(dep_generator.scripts) do
                if not script_config.private then
                    table.insert(scripts, script_config)
                end
            end

            -- extract dep_files
            for __, dep_file in ipairs(dep_generator.dep_files) do
                table.insert(dep_files, dep_file)
            end
        end
    end

    -- call codegen scripts
    utils.on_changed(function (change_info)
        _mako_render(target, scripts, dep_files, opt)
    end, {
        cache_file = utils.depend_file(path.join("codegen", target:name(), "codegen.mako")),
        files = dep_files,
        use_sha = true,
        post_scan = function ()
            local batchinfo = target:data(_codegen_data_batch_name)
            local gendir = batchinfo.gendir
            local headers = os.files(path.join(gendir, "**.h"))
            local sources = os.files(path.join(gendir, "**.cpp"))
            for _, source in ipairs(sources) do
                table.insert(headers, source)
            end
            return headers
        end
    })
end

function is_env_complete()
    -- return _meta and _python
    return _meta and _bun
end