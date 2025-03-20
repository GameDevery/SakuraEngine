#pragma once
#include "SkrBase/config.h"
#include "SkrContainers/map.hpp"
#include "SkrRTTR/script_binder.hpp"
#include "v8-isolate.h"
#include "v8-platform.h"
#include "v8_bind_data.hpp"
#include "v8-primitive.h"

namespace skr
{
struct SKR_V8_API V8BindManager : IScriptMixinCore {
    // ctor & dtor
    V8BindManager();
    ~V8BindManager();

    // delate copy & move
    V8BindManager(const V8BindManager&)            = delete;
    V8BindManager(V8BindManager&&)                 = delete;
    V8BindManager& operator=(const V8BindManager&) = delete;
    V8BindManager& operator=(V8BindManager&&)      = delete;

    // clean up data
    void cleanup_templates();
    void cleanup_bind_cores();

    // bind object
    V8BindCoreObject* translate_object(::skr::ScriptbleObject* obj);
    void              mark_object_deleted(::skr::ScriptbleObject* obj);

    // bind value
    V8BindCoreValue* create_value(const RTTRType* type, const void* data);
    V8BindCoreValue* translate_value_field(const RTTRType* type, const void* data, V8BindCoreRecordBase* owner);

    // query template
    v8::Local<v8::ObjectTemplate>   get_enum_template(const RTTRType* type);
    v8::Local<v8::FunctionTemplate> get_record_template(const RTTRType* type);

    // => IScriptMixinCore API
    void on_object_destroyed(ScriptbleObject* obj) override;
    // => IScriptMixinCore API

private:
    // template helper
    v8::Local<v8::FunctionTemplate> _make_template_object(ScriptBinderObject* object_binder);
    v8::Local<v8::FunctionTemplate> _make_template_value(ScriptBinderValue* value_binder);

    void _fill_record_template(
        ScriptBinderRecordBase*         binder,
        V8BindDataRecordBase*           bind_data,
        v8::Local<v8::FunctionTemplate> ctor_template
    );

    // bind methods
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
    // binder manager
    ScriptBinderManager _binder_mgr;

    // template & bind data
    Map<const RTTRType*, V8BindDataRecordBase*> _record_templates;
    Map<const RTTRType*, V8BindDataEnum*>       _enum_templates;

    // bind cores & objects
    Map<::skr::ScriptbleObject*, V8BindCoreObject*> _alive_objects;
    Vector<V8BindCoreObject*>                       _deleted_objects;
    Map<void*, V8BindCoreValue*>                    _script_created_values;
};
} // namespace skr