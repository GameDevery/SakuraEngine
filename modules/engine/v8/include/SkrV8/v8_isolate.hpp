#pragma once
#include "SkrRTTR/script_binder.hpp"
#include <SkrRTTR/stack_proxy.hpp>
#include "v8-isolate.h"
#include "v8-platform.h"
#include "v8_bind_data.hpp"
#include "v8-primitive.h"
#ifndef __meta__
    #include "SkrV8/v8_isolate.generated.h"
#endif

namespace skr
{
struct V8Context;
struct V8Module;

// clang-format off
sreflect_struct(guid = "8aea5942-6e5c-4711-9502-83f252faa231")
SKR_V8_API V8Isolate: IScriptMixinCore {
    // clang-format on
    SKR_GENERATE_BODY()

    friend struct V8Context;
    friend struct V8Value;
    friend struct V8Module;

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

    // isolate operators
    void pump_message_loop();
    void gc(bool full = true);

    // getter
    inline v8::Isolate*               v8_isolate() const { return _isolate; }
    inline ScriptBinderManager&       script_binder_manger() { return _binder_mgr; }
    inline const ScriptBinderManager& script_binder_manger() const { return _binder_mgr; }

    // bind object
    V8BindCoreObject* translate_object(::skr::ScriptbleObject* obj);
    void              mark_object_deleted(::skr::ScriptbleObject* obj);

    // bind value
    V8BindCoreValue* create_value(const RTTRType* type, const void* source_data = nullptr);
    V8BindCoreValue* translate_value_field(const RTTRType* type, const void* data, V8BindCoreRecordBase* owner);

    // query template
    v8::Local<v8::ObjectTemplate>   get_or_add_enum_template(const RTTRType* type);
    v8::Local<v8::FunctionTemplate> get_or_add_record_template(const RTTRType* type);

    // clean up bind data
    void cleanup_templates();
    void cleanup_bind_cores();

    // convert
    v8::Local<v8::Value> to_v8(TypeSignatureView sig_view, const void* data);
    bool                 to_native(TypeSignatureView sig_view, void* data, v8::Local<v8::Value> v8_value, bool is_init);

    // invoke script
    bool invoke_v8(
        v8::Local<v8::Value>    v8_this,
        v8::Local<v8::Function> v8_func,
        span<const StackProxy>  params,
        StackProxy              return_value
    );

    // => IScriptMixinCore API
    void on_object_destroyed(ScriptbleObject* obj) override;
    bool try_invoke_mixin(ScriptbleObject* obj, StringView name, const span<const StackProxy> params, StackProxy ret) override;
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

    // module callback
    static v8::MaybeLocal<v8::Promise> _dynamic_import_module(
        v8::Local<v8::Context>    context,
        v8::Local<v8::Data>       host_defined_options,
        v8::Local<v8::Value>      resource_name,
        v8::Local<v8::String>     specifier,
        v8::Local<v8::FixedArray> import_assertions
    );

    // uniform new
    template <typename T>
    inline T* _new_bind_data()
    {
        auto* result    = SkrNew<T>();
        result->manager = this;
        return result;
    }
    template <typename T>
    inline T* _new_bind_core()
    {
        auto* result        = SkrNew<T>();
        result->skr_isolate = this;
        return result;
    }

    // bind methods
    static void _gc_callback(const ::v8::WeakCallbackInfo<V8BindCoreRecordBase>& data);
    static void _call_ctor(const ::v8::FunctionCallbackInfo<::v8::Value>& info);
    static void _call_mixin(const ::v8::FunctionCallbackInfo<::v8::Value>& info);
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

    // convert helper
    v8::Local<v8::Value> _to_v8(
        ScriptBinderRoot binder,
        void*            native_data
    );
    bool _to_native(
        ScriptBinderRoot     binder,
        void*                native_data,
        v8::Local<v8::Value> v8_value,
        bool                 is_init
    );
    v8::Local<v8::Value> _to_v8_primitive(
        const ScriptBinderPrimitive& binder,
        void*                        native_data
    );
    void _init_primitive(
        const ScriptBinderPrimitive& binder,
        void*                        native_data
    );
    bool _to_native_primitive(
        const ScriptBinderPrimitive& binder,
        v8::Local<v8::Value>         v8_value,
        void*                        native_data,
        bool                         is_init
    );
    v8::Local<v8::Value> _to_v8_mapping(
        const ScriptBinderMapping& binder,
        void*                      obj
    );
    bool _to_native_mapping(
        const ScriptBinderMapping& binder,
        v8::Local<v8::Value>       v8_value,
        void*                      native_data,
        bool                       is_init
    );
    v8::Local<v8::Value> _to_v8_object(
        const ScriptBinderObject& binder,
        void*                     native_data
    );
    bool _to_native_object(
        const ScriptBinderObject& binder,
        v8::Local<v8::Value>      v8_value,
        void*                     native_data,
        bool                      is_init
    );
    v8::Local<v8::Value> _to_v8_value(
        const ScriptBinderValue& binder,
        void*                    native_data
    );
    bool _to_native_value(
        const ScriptBinderValue& binder,
        v8::Local<v8::Value>     v8_value,
        void*                    native_data,
        bool                     is_init
    );

    // field convert helper
    static void* _get_field_address(
        const RTTRFieldData* field,
        const RTTRType*      field_owner,
        const RTTRType*      obj_type,
        void*                obj
    );
    bool _set_field_value_or_object(
        const ScriptBinderField& binder,
        v8::Local<v8::Value>     v8_value,
        V8BindCoreRecordBase*    bind_core
    );
    bool _set_field_mapping(
        const ScriptBinderField& binder,
        v8::Local<v8::Value>     v8_value,
        void*                    obj,
        const RTTRType*          obj_type
    );
    v8::Local<v8::Value> _get_field_value_or_object(
        const ScriptBinderField& binder,
        V8BindCoreRecordBase*    bind_core
    );
    v8::Local<v8::Value> _get_field_mapping(
        const ScriptBinderField& binder,
        const void*              obj,
        const RTTRType*          obj_type
    );
    bool _set_static_field(
        const ScriptBinderStaticField& binder,
        v8::Local<v8::Value>           v8_value
    );
    v8::Local<v8::Value> _get_static_field(
        const ScriptBinderStaticField& binder
    );

    // param & return convert helper
    void _push_param(
        DynamicStack&            stack,
        const ScriptBinderParam& param_binder,
        v8::Local<v8::Value>     v8_value
    );
    void _push_param_pure_out(
        DynamicStack&            stack,
        const ScriptBinderParam& param_binder
    );
    v8::Local<v8::Value> _read_return(
        DynamicStack&                    stack,
        const Vector<ScriptBinderParam>& params_binder,
        const ScriptBinderReturn&        return_binder,
        uint32_t                         solved_return_count
    );
    v8::Local<v8::Value> _read_return(
        DynamicStack&             stack,
        const ScriptBinderReturn& return_binder
    );
    v8::Local<v8::Value> _read_return_from_out_param(
        DynamicStack&            stack,
        const ScriptBinderParam& param_binder
    );

    // invoke helper
    bool _call_native(
        const ScriptBinderCtor&                        binder,
        const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack,
        void*                                          obj
    );
    bool _call_native(
        const ScriptBinderMethod&                      binder,
        const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack,
        void*                                          obj,
        const RTTRType*                                obj_type
    );
    bool _call_native(
        const ScriptBinderStaticMethod&                binder,
        const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack
    );

    // TODO. call native helper
    // param helper for call script
    // v8::Local<v8::Value> _translate_param(StackProxy proxy);
    // void                 _translate_return(StackProxy proxy, v8::Local<v8::Value> v8_value);

private:
    // isolate data
    v8::Isolate*              _isolate;
    v8::Isolate::CreateParams _isolate_create_params;

    // modules
    Map<String, V8Module*> _modules       = {};
    Map<int, V8Module*>    _to_skr_module = {};

    // binder manager
    ScriptBinderManager _binder_mgr;

    // template & bind data
    Map<const RTTRType*, V8BindDataRecordBase*> _record_templates;
    Map<const RTTRType*, V8BindDataEnum*>       _enum_templates;

    // bind cores & objects
    Map<ScriptbleObject*, V8BindCoreObject*> _alive_objects;
    Vector<V8BindCoreObject*>                _deleted_objects;
    Map<void*, V8BindCoreValue*>             _script_created_values;
    Map<void*, V8BindCoreValue*>             _static_field_values;
};
} // namespace skr

// global init
namespace skr
{
SKR_V8_API void init_v8();
SKR_V8_API void shutdown_v8();
SKR_V8_API v8::Platform& get_v8_platform();
} // namespace skr