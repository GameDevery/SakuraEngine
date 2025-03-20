#pragma once
#include "SkrBase/config.h"
#include "SkrContainers/map.hpp"
#include "SkrRTTR/script_binder.hpp"
#include "v8-isolate.h"
#include "v8-platform.h"
#include "v8_bind_data.hpp"
#include "v8-primitive.h"

// TODO. 使用 p-impl 实现多后端，并且对外层屏蔽对 v8 context 的操作，一切操作通过反射注入
// TODO. custom getter/setter
namespace skr
{
struct V8Context;

struct SKR_V8_API V8Isolate : IScriptMixinCore {
    friend struct V8Context;

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

    // => IScriptMixinCore API
    void on_object_destroyed(ScriptbleObject* obj) override;
    // => IScriptMixinCore API

private:
    // make template
    v8::Local<v8::ObjectTemplate>   _get_enum_template(const RTTRType* type);
    v8::Local<v8::FunctionTemplate> _get_record_template(const RTTRType* type);
    v8::Local<v8::FunctionTemplate> _make_template_object(ScriptBinderObject* binder);
    v8::Local<v8::FunctionTemplate> _make_template_value(ScriptBinderValue* binder);

    void _fill_record_data(
        ScriptBinderRecordBase*         binder,
        V8BindDataRecordBase*           bind_data,
        v8::Local<v8::FunctionTemplate> ctor_template
    );

    // bind helpers
    static void _gc_callback(const ::v8::WeakCallbackInfo<V8BindCoreRecordBase>& data);
    static void _call_ctor(const ::v8::FunctionCallbackInfo<::v8::Value>& info);
    static void _call_method(const ::v8::FunctionCallbackInfo<::v8::Value>& info);
    static void _call_static_method(const ::v8::FunctionCallbackInfo<::v8::Value>& info);
    static void _get_field(const ::v8::FunctionCallbackInfo<::v8::Value>& info);
    static void _set_field(const ::v8::FunctionCallbackInfo<::v8::Value>& info);
    static void _get_static_field(const ::v8::FunctionCallbackInfo<::v8::Value>& info);
    static void _set_static_field(const ::v8::FunctionCallbackInfo<::v8::Value>& info);
    static void _get_prop(const ::v8::FunctionCallbackInfo<::v8::Value>& info);
    static void _set_prop(const ::v8::FunctionCallbackInfo<::v8::Value>& info);
    static void _get_static_prop(const ::v8::FunctionCallbackInfo<::v8::Value>& info);
    static void _set_static_prop(const ::v8::FunctionCallbackInfo<::v8::Value>& info);
    static void _enum_to_string(const ::v8::FunctionCallbackInfo<::v8::Value>& info);
    static void _enum_from_string(const ::v8::FunctionCallbackInfo<::v8::Value>& info);

private:
    // isolate data
    ::v8::Isolate*              _isolate;
    ::v8::Isolate::CreateParams _isolate_create_params;

    // binder manager
    ScriptBinderManager _binder_mgr;

    // templates
    Map<const RTTRType*, V8BindDataRecordBase*> _record_templates;
    Map<const RTTRType*, V8BindDataEnum*>       _enum_templates;

    // bind data
    Map<::skr::ScriptbleObject*, V8BindCoreObject*> _alive_objects;
    Vector<V8BindCoreObject*>                       _deleted_objects;
    Map<void*, V8BindCoreValue*>                    _script_created_values;
};
} // namespace skr

// global init
namespace skr
{
SKR_V8_API void init_v8();
SKR_V8_API void shutdown_v8();
} // namespace skr