#include "SkrV8/v8_context.hpp"
#include <SkrV8/v8_isolate.hpp>
#include <SkrV8/v8_module.hpp>
#include <SkrCore/log.hpp>
#include "SkrCore/cli.hpp"
#include <V8Playground/debug.hpp>
#include <V8Playground/cmd_args.hpp>

int main(int argc, char* argv[])
{
    // parse args
    skr::CmdParser parser;
    v8_play::MainCommand cmd;
    parser.main_cmd(&cmd, {
        .help = u8"V8Playground [options]",
    });
    parser.parse(argc, argv);
    
    // init v8
    skr::init_v8();
    skr::V8Isolate isolate;
    isolate.init();

    // load script
    skr::String script;
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
        skr::V8Context context(&isolate);
        skr::V8Module  module(&isolate);
        context.init();

        // register module
        module.name(u8"fuck");
        module.register_type<v8_play::Debug>(u8"");
        module.finalize();

        // register env
        context.register_type<v8_play::Debug>(u8"");
        context.finalize_register();

        // execute script
        context.exec_module(script);

        context.shutdown();
    }

    // shutdown v8
    isolate.shutdown();
    skr::shutdown_v8();
    return 0;
}