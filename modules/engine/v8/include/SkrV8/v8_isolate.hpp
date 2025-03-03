#pragma once
#include "SkrBase/config.h"
#include "SkrContainers/map.hpp"
#include "v8-isolate.h"
#include "v8-platform.h"
#include "v8_bind_data.hpp"
#include "v8-primitive.h"

namespace skr::rttr
{
struct Type;
}

namespace skr::v8
{
struct V8Context;

struct SKR_V8_API V8Isolate {
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

    // register type
    void make_record_template(::skr::rttr::Type* type);
    void inject_templates_into_context(::v8::Global<::v8::Context> context);

    // bind object
    V8BindRecordCore* translate_record(::skr::rttr::ScriptbleObject* obj);
    void              mark_record_deleted(::skr::rttr::ScriptbleObject* obj);

private:
    // bind helpers
    static void _gc_callback(const ::v8::WeakCallbackInfo<V8BindRecordCore>& data);
    static void _call_ctor(const ::v8::FunctionCallbackInfo<::v8::Value>& info);
    static void _call_method(const ::v8::FunctionCallbackInfo<::v8::Value>& info);
    static void _call_static_method(const ::v8::FunctionCallbackInfo<::v8::Value>& info);
    static void _get_field(const ::v8::FunctionCallbackInfo<::v8::Value>& info);
    static void _set_field(const ::v8::FunctionCallbackInfo<::v8::Value>& info);
    static void _get_static_field(const ::v8::FunctionCallbackInfo<::v8::Value>& info);
    static void _set_static_field(const ::v8::FunctionCallbackInfo<::v8::Value>& info);

private:
    // isolate data
    ::v8::Isolate*              _isolate;
    ::v8::Isolate::CreateParams _isolate_create_params;

    // templates
    Map<::skr::rttr::Type*, V8BindRecordData*> _record_templates;

    // bind data
    Map<::skr::rttr::ScriptbleObject*, V8BindRecordCore*> _alive_records;
    Vector<V8BindRecordCore*>                             _deleted_records;
};
} // namespace skr::v8

// global init
namespace skr::v8
{
SKR_V8_API void init_v8();
SKR_V8_API void shutdown_v8();
} // namespace skr::v8