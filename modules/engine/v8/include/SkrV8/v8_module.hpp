#pragma once
#include "SkrBase/config.h"
#include "SkrBase/meta.h"
#include "v8-persistent-handle.h"
#include "v8-script.h"
#include <SkrContainers/string.hpp>

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

    // finalize & shutdown
    void finalize();
    void shutdown();

private:
    // owner
    V8Isolate* _isolate = nullptr;

    // module info
    String _name;

    // module data
    v8::Persistent<v8::Module> _module;
};
} // namespace skr