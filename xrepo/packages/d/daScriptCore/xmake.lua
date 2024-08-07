package("daScriptCore")
    set_homepage("https://dascript.org/")
    set_description("daScript - high-performance statically strong typed scripting language")

    add_versions("2023.4.26-skr.1", "582a5ec3414951b91bf8ee8aa413688c1d07014a2602ad6a63b1ef29c3b61a7c")

    add_defines("URI_STATIC_BUILD")
    add_defines("URIPARSER_BUILD_CHAR")
    on_load(function (package)
        if package:is_debug() then
            package:add("defines", "DAS_SMART_PTR_TRACKER=1")
            package:add("defines", "DAS_SMART_PTR_MAGIC=1")
        else 
            package:add("defines", "DAS_DEBUGGER=0")
            package:add("defines", "DAS_SMART_PTR_TRACKER=0")
            package:add("defines", "DAS_SMART_PTR_MAGIC=0")
            package:add("defines", "DAS_FREE_LIST=1")
            package:add("defines", "DAS_FUSION=2")
        end
    end)

    on_install(function (package)
        os.mkdir(package:installdir())
        os.cp(path.join(package:scriptdir(), "port", "daScript", "include"), ".")
        os.cp(path.join(package:scriptdir(), "port", "daScript", "src"), ".")
        os.cp(path.join(package:scriptdir(), "port", "daScript", "test"), ".")
        os.cp(path.join(package:scriptdir(), "port", "daScript", "3rdparty"), ".")
        os.cp(path.join(package:scriptdir(), "port", "xmake.lua"), "xmake.lua")

        local configs = {}
        import("package.tools.xmake").install(package, configs)
    end)

    on_test(function (package)
        assert(package:check_cxxsnippets({test = [[
            #include "daScript/daScript.h"
            using namespace das;
            const char * tutorial_text = R""""(
            [export]
            def test
                print("this is nano tutorial\n")
            )"""";
            static void test() 
            {
                // request all da-script built in modules
                NEED_ALL_DEFAULT_MODULES;
                // Initialize modules
                Module::Initialize();
                // make file access, introduce string as if it was a file
                auto fAccess = make_smart<FsFileAccess>();
                auto fileInfo = make_unique<TextFileInfo>(tutorial_text, uint32_t(strlen(tutorial_text)), false);
                fAccess->setFileInfo("dummy.das", das::move(fileInfo));
                // compile script
                TextPrinter tout;
                ModuleGroup dummyLibGroup;
                auto program = compileDaScript("dummy.das", fAccess, tout, dummyLibGroup);
                if ( program->failed() ) return;
                // create context
                Context ctx(program->getContextStackSize());
                if ( !program->simulate(ctx, tout) ) return;
                // find function. its up to application to check, if function is not null
                auto function = ctx.findFunction("test");
                if ( !function ) return;
                // call context function
                ctx.evalWithCatch(function, nullptr);
                // shut-down daScript, free all memory
                Module::Shutdown();
            }
        ]]}, {configs = {languages = "c++17"}}))
    end)


