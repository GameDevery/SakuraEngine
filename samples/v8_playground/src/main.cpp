#include "SkrV8/v8_context.hpp"
#include <SkrV8/v8_isolate.hpp>
#include <SkrCore/log.hpp>
#include <V8Playground/debug.hpp>

int main(int argc, char* argv[])
{
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
        context.init();

        // register env
        context.register_type<v8_play::Debug>();

        // execute script
        context.exec_script(script);

        context.shutdown();
    }

    // shutdown v8
    isolate.shutdown();
    skr::shutdown_v8();
    return 0;
}