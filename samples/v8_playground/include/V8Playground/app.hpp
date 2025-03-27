#pragma once
#include "SkrV8/v8_context.hpp"
#include <SkrV8/v8_isolate.hpp>
#include <SkrV8/v8_module.hpp>
#include <SkrCore/log.hpp>

namespace skr
{
struct V8PlaygroundApp {
    // ctor & dtor
    V8PlaygroundApp();
    ~V8PlaygroundApp();

    // env init & shutdown
    static void env_init();
    static void env_shutdown();

    // init & shutdown
    void init();
    void shutdown();

    // load native
    void load_native_types();

    // run script
    bool run_script(StringView script_path);

    // dump types
    bool dump_types(StringView output_dir);

private:
    // helper
    String _load_from_file(StringView _path);
    bool   _save_to_file(StringView _path, StringView _content);

private:
    V8Isolate* _isolate      = nullptr;
    V8Context* _main_context = nullptr;
};
} // namespace skr