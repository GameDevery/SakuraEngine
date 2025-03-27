#pragma once
#include "SkrBase/config.h"
#include "SkrBase/meta.h"
#include "v8-persistent-handle.h"
#include "v8-script.h"
#include <SkrRTTR/script_tools.hpp>

namespace skr
{
struct V8Isolate;

struct SKR_V8_API V8Module {
    // ctor & dtor
    V8Module(V8Isolate* isolate);
    ~V8Module();

    // delate copy & move
    V8Module(const V8Module&)            = delete;
    V8Module(V8Module&&)                 = delete;
    V8Module& operator=(const V8Module&) = delete;
    V8Module& operator=(V8Module&&)      = delete;

    // build api
    void name(StringView name);
    bool build(FunctionRef<void(ScriptModule& module)> build_func);

    // shutdown
    void shutdown();

    // getter
    inline const String&       name() const { return _name; }
    inline const ScriptModule& module_info() const { return _module_info; }
    v8::Local<v8::Module>      v8_module() const;

private:
    // eval callback
    static v8::MaybeLocal<v8::Value> _eval_callback(
        v8::Local<v8::Context> context,
        v8::Local<v8::Module>  module
    );

private:
    // owner
    V8Isolate* _isolate = nullptr;

    // module info
    String       _name;
    ScriptModule _module_info;

    // module data
    v8::Persistent<v8::Module> _module;
};
} // namespace skr