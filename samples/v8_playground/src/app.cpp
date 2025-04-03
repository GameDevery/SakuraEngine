#include "SkrV8/ts_define_exporter.hpp"
#include <V8Playground/app.hpp>
#include <filesystem>

namespace skr
{
// ctor & dtor
V8PlaygroundApp::V8PlaygroundApp()
{
}
V8PlaygroundApp::~V8PlaygroundApp()
{
    shutdown();
}

// env init & shutdown
void V8PlaygroundApp::env_init()
{
    init_v8();
}
void V8PlaygroundApp::env_shutdown()
{
    shutdown_v8();
}

// init & shutdown
void V8PlaygroundApp::init()
{
    shutdown();
    _isolate = SkrNew<V8Isolate>();
    _isolate->init();
    _main_context = SkrNew<V8Context>(_isolate);
    _main_context->init();
}
void V8PlaygroundApp::shutdown()
{
    if (_isolate)
    {
        _main_context->shutdown();
        SkrDelete(_main_context);
        _isolate->shutdown();
        SkrDelete(_isolate);
        _isolate      = nullptr;
        _main_context = nullptr;
    }
}

// debug
void V8PlaygroundApp::init_debugger(int port)
{
    _websocket_server.init(port);
    _inspector_client.server = &_websocket_server;
    _inspector_client.init(_isolate);
    _inspector_client.notify_context_created(_main_context, u8"main");
}
void V8PlaygroundApp::shutdown_debugger()
{
    _inspector_client.shutdown();
    _inspector_client.server = nullptr;
    _websocket_server.shutdown();
}
void V8PlaygroundApp::pump_debugger_messages()
{
    _websocket_server.pump_messages();
}
void V8PlaygroundApp::wait_for_debugger_connected()
{
    while (!_inspector_client.is_connected())
    {
        // pump v8 messages
        _isolate->pump_message_loop();

        // pump net messages
        _websocket_server.pump_messages();

        // sleep for a while
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

// load native
void V8PlaygroundApp::load_native_types()
{
    load_all_types();
    _main_context->build_global_export([](ScriptModule& module) {
        each_types_of_module(u8"V8Playground", [&](const RTTRType* type) -> bool {
            bool do_export =
                (type->is_record() && flag_all(type->record_flag(), ERTTRRecordFlag::ScriptVisible)) ||
                (type->is_enum() && flag_all(type->enum_flag(), ERTTREnumFlag::ScriptVisible));

            if (do_export)
            {
                auto ns = type->name_space_str();
                ns.remove_prefix(u8"skr");
                module.register_type(type, ns);
            }
            return true;
        });
    });
}

// run script
bool V8PlaygroundApp::run_script(StringView script_path)
{
    // load script
    String script = _load_from_file(script_path);
    if (script.is_empty())
    {
        SKR_LOG_FMT_ERROR(u8"Failed to load script: {}", script_path);
        return false;
    }

    // run script
    _main_context->exec_module(script, script_path);

    return true;
}

// dump types
bool V8PlaygroundApp::dump_types(StringView output_dir)
{
    // get name
    std::filesystem::path out_dir_path(output_dir.data_raw());
    std::filesystem::path out_file_path = out_dir_path / "global.d.ts";

    // create dir
    if (!std::filesystem::exists(out_dir_path))
    {
        if (!std::filesystem::create_directories(out_dir_path))
        {
            SKR_LOG_FMT_ERROR(u8"Failed to create dir: {}", out_dir_path.string());
            return false;
        }
    }
    else if (!std::filesystem::is_directory(out_dir_path))
    {
        SKR_LOG_FMT_ERROR(u8"Path is not a directory: {}", out_dir_path.string());
        return false;
    }

    // do export
    TSDefineExporter exporter;
    exporter.module = &_main_context->global_module();
    return _save_to_file(String::From(out_file_path.c_str()), exporter.generate_global());
}

// helper
String V8PlaygroundApp::_load_from_file(StringView path)
{
    String result;
    auto   handle = fopen(path.data_raw(), "rb");
    if (handle)
    {
        // resize string to file size
        fseek(handle, 0, SEEK_END);
        auto size = ftell(handle);
        fseek(handle, 0, SEEK_SET);
        result.resize_unsafe(size);

        // read file to string
        fread(result.data_w(), 1, size, handle);
        fclose(handle);
    }
    else
    {
        return {};
    }
    return result;
}
bool V8PlaygroundApp::_save_to_file(StringView _path, StringView _content)
{
    auto handle = fopen(_path.data_raw(), "wb");
    if (!handle) { return false; }

    fwrite(_content.data(), 1, _content.size(), handle);
    fclose(handle);
    return true;
}
} // namespace skr