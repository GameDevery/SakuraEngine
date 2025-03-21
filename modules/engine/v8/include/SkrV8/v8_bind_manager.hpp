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
    V8BindCoreValue* create_value(const RTTRType* type, const void* source_data = nullptr);
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

    // uniform new
    template <typename T>
    inline T* _new_bind_data()
    {
        auto* result    = SkrNew<T>();
        result->manager = this;
        return result;
    }

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