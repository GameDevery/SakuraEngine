#include "SkrV8/ts_def_builder.hpp"
#include "v8-exception.h"
#include <V8Playground/app.hpp>
#include "SkrOS/filesystem.hpp"
#include <SkrV8/v8_vfs.hpp>

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
}
void V8PlaygroundApp::shutdown()
{
    if (_isolate)
    {
        _isolate->shutdown();
        SkrDelete(_isolate);
        _isolate = nullptr;
    }
}
void V8PlaygroundApp::setup_vfs(StringView path)
{
    auto vfs = RC<V8VFSSystemFS>::New();
    vfs->root_path = path;
    _isolate->vfs = vfs;
}

// debug
void V8PlaygroundApp::init_debugger(int port)
{
    _isolate->init_debugger(port);
}
void V8PlaygroundApp::shutdown_debugger()
{
    _isolate->shutdown_debugger();
}
void V8PlaygroundApp::pump_debugger_messages()
{
    _isolate->pump_debugger_messages();
}
void V8PlaygroundApp::wait_for_debugger_connected()
{
    _isolate->wait_for_debugger_connected();
}

// load native
void V8PlaygroundApp::load_native_types()
{
    load_all_types();
    _isolate->main_context()->build_export([](V8VirtualModule& module) {
        each_types_of_module(u8"V8Playground", [&](const RTTRType* type) -> bool {
            bool do_export =
                (type->is_record() && flag_all(type->record_flag(), ERTTRRecordFlag::ScriptVisible)) ||
                (type->is_enum() && flag_all(type->enum_flag(), ERTTREnumFlag::ScriptVisible));

            if (do_export)
            {
                auto ns = type->name_space_str();
                ns.remove_prefix(u8"skr");
                module.raw_register_type(type->type_id(), ns);
            }
            return true;
        });
    });
}

// run script
bool V8PlaygroundApp::run_script(StringView script_path)
{
    _isolate->main_context()->exec_file(script_path);
    return true;
}

// dump types
bool V8PlaygroundApp::dump_types(StringView output_dir)
{
    // get name
    skr::Path out_dir_path{ output_dir };
    skr::Path out_file_path = out_dir_path / u8"global.d.ts";

    // create dir
    if (!skr::fs::Directory::exists(out_dir_path))
    {
        if (!skr::fs::Directory::create(out_dir_path, true))
        {
            SKR_LOG_FMT_ERROR(u8"Failed to create dir: {}", out_dir_path.string());
            return false;
        }
    }
    else if (!skr::fs::Directory::is_directory(out_dir_path))
    {
        SKR_LOG_FMT_ERROR(u8"Path is not a directory: {}", out_dir_path.string());
        return false;
    }

    // do export
    TSDefBuilder builder;
    builder.virtual_module = &_isolate->main_context()->virtual_module();
    return _save_to_file(out_file_path.string(), builder.generate_global());
}

// helper
String V8PlaygroundApp::_load_from_file(StringView path)
{
    String result;
    auto handle = fopen(path.data_raw(), "rb");
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