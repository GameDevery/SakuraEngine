#pragma once
#include <SkrV8/v8_fwd.hpp>
#include <SkrV8/v8_bind.hpp>
#include <SkrV8/v8_virtual_module.hpp>
#include <SkrV8/v8_value.hpp>
#include <SkrCore/memory/rc.hpp>
#include <SkrRTTR/script/script_binder.hpp>
#include <SkrRTTR/script/stack_proxy.hpp>

// v8 includes
#include <v8-isolate.h>
#include <v8-platform.h>
#include <v8-primitive.h>

namespace skr
{
struct SKR_V8_API V8Context {
    SKR_RC_IMPL();

    friend struct V8Isolate;

    // ctor & dtor
    V8Context();
    ~V8Context();

    // delete copy & move
    V8Context(const V8Context&)            = delete;
    V8Context(V8Context&&)                 = delete;
    V8Context& operator=(const V8Context&) = delete;
    V8Context& operator=(V8Context&&)      = delete;

    // getter
    ::v8::Global<::v8::Context> v8_context() const;
    inline V8Isolate*           isolate() const { return _isolate; }
    inline const String&        name() const { return _name; }

    // build export
    void build_export(FunctionRef<void(V8VirtualModule&)> build_func);
    bool is_export_built() const;
    void clear_export();

    // set & get global value
    V8Value get_global(StringView name);

    // temp api
    void temp_run_script(StringView script);

private:
    // init & shutdown
    void _init(V8Isolate* isolate, String name);
    void _shutdown();

private:
    V8Isolate*                  _isolate        = nullptr;
    v8::Persistent<v8::Context> _context        = {};
    String                      _name           = {};
    V8VirtualModule             _virtual_module = {};
};
} // namespace skr