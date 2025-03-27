#include "SkrV8/v8_context.hpp"
#include <SkrV8/v8_isolate.hpp>
#include <SkrV8/v8_module.hpp>
#include <SkrCore/log.hpp>
#include "SkrCore/cli.hpp"
#include <V8Playground/debug.hpp>
#include <V8Playground/cmd_args.hpp>

int main(int argc, char* argv[])
{
    using namespace skr;

    // parse args
    CmdParser   parser;
    MainCommand cmd;
    parser.main_cmd(
        &cmd,
        {
            .help  = u8"V8 Playground",
            .usage = u8"V8Playground [options]",
        }
    );
    parser.parse(argc, argv);

    return 0;

    // init v8
    init_v8();
    V8Isolate isolate;
    isolate.init();

    // load script
    String script;
    {
        auto handle = fopen("../script/main.js", "rb");
        if (handle)
        {
            // resize string to file size
            fseek(handle, 0, SEEK_END);
            auto size = ftell(handle);
            fseek(handle, 0, SEEK_SET);
            script.resize_unsafe(size);

            // read file to string
            fread(script.data_w(), 1, size, handle);
            fclose(handle);
        }
        else
        {
            SKR_LOG_FMT_ERROR(u8"Failed to load script");
            return -1;
        }
    }

    // run script
    {
        V8Context context(&isolate);
        V8Module  module(&isolate);
        context.init();

        // register module
        module.name(u8"fuck");
        module.build([](ScriptModule& module) {
            module.register_type<Debug>(u8"");
        });

        // register env
        context.build_global_export([](ScriptModule& module) {
            module.register_type<Debug>(u8"");
        });

        // execute script
        context.exec_module(script);

        context.shutdown();
    }

    // shutdown v8
    isolate.shutdown();
    skr::shutdown_v8();
    return 0;
}