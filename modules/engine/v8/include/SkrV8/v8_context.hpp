#pragma once
#include "SkrBase/config.h"
#include "SkrContainers/string.hpp"
#include "SkrRTTR/scriptble_object.hpp"
#include "v8-context.h"
#include "v8-persistent-handle.h"

namespace skr
{
struct V8Isolate;

struct SKR_V8_API V8Context {
    // ctor & dtor
    V8Context(V8Isolate* isolate);
    ~V8Context();

    // delete copy & move
    V8Context(const V8Context&)            = delete;
    V8Context(V8Context&&)                 = delete;
    V8Context& operator=(const V8Context&) = delete;
    V8Context& operator=(V8Context&&)      = delete;

    // init & shutdown
    void init();
    void shutdown();

    // getter
    ::v8::Global<::v8::Context> v8_context() const;
    
    // set global value
    void set_global(StringView name, uint32_t v);
    void set_global(StringView name, int32_t v);
    void set_global(StringView name, uint64_t v);
    void set_global(StringView name, int64_t v);
    void set_global(StringView name, double v);
    void set_global(StringView name, bool v);
    void set_global(StringView name, StringView v);
    void set_global(StringView name, skr::rttr::ScriptbleObject* obj);

    // run script
    void exec_script(StringView script);

private:
    // owner
    V8Isolate* _isolate;

    // context data
    ::v8::Persistent<::v8::Context> _context;
};
} // namespace skr