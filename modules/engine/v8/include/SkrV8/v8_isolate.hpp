#pragma once
#include "SkrBase/config.h"
#include "SkrContainers/map.hpp"
#include "SkrRTTR/script_binder.hpp"
#include "v8-isolate.h"
#include "v8-platform.h"
#include "v8_bind_data.hpp"
#include "v8-primitive.h"
#include "v8_bind_manager.hpp"

namespace skr
{
struct V8Context;

struct SKR_V8_API V8Isolate {
    friend struct V8Context;
    friend struct V8Value;

    // ctor & dtor
    V8Isolate();
    ~V8Isolate();

    // delate copy & move
    V8Isolate(const V8Isolate&)            = delete;
    V8Isolate(V8Isolate&&)                 = delete;
    V8Isolate& operator=(const V8Isolate&) = delete;
    V8Isolate& operator=(V8Isolate&&)      = delete;

    // init & shutdown
    void init();
    void shutdown();

    // getter
    inline ::v8::Isolate* v8_isolate() const { return _isolate; }

    // make context
    V8Context* make_context();

    // operator isolate
    void gc(bool full = true);

    // bind object
    V8BindCoreObject* translate_object(::skr::ScriptbleObject* obj);
    void              mark_object_deleted(::skr::ScriptbleObject* obj);

    // bind value
    V8BindCoreValue* create_value(const RTTRType* type, const void* data);
    V8BindCoreValue* translate_value_field(const RTTRType* type, const void* data, V8BindCoreRecordBase* owner);

private:
    // make template
    void                            register_mapping_type(const RTTRType* type);
    v8::Local<v8::ObjectTemplate>   _get_enum_template(const RTTRType* type);
    v8::Local<v8::FunctionTemplate> _get_record_template(const RTTRType* type);

private:
    // isolate data
    ::v8::Isolate*              _isolate;
    ::v8::Isolate::CreateParams _isolate_create_params;

    // bind manager
    V8BindManager _bind_manager;
};
} // namespace skr

// global init
namespace skr
{
SKR_V8_API void init_v8();
SKR_V8_API void shutdown_v8();
} // namespace skr