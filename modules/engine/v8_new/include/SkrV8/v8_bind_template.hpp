#pragma once
#include <SkrRTTR/script/scriptble_object.hpp>

// v8 includes
#include <v8-persistent-handle.h>
#include <v8-template.h>

namespace skr
{
struct V8BindProxy;
struct V8BPValue;
struct V8BPObject;

struct IV8BindManager {
    virtual ~IV8BindManager() = default;

    // bind proxy management
    virtual void add_bind_proxy(
        void*        native_ptr,
        V8BindProxy* bind_proxy
    ) = 0;
    virtual void remove_bind_proxy(
        void*        native_ptr,
        V8BindProxy* bind_proxy
    ) = 0;
    virtual V8BindProxy* find_bind_proxy(
        void* native_ptr
    ) const = 0;

    // 用于缓存调用期间为参数创建的临时 bind proxy
    virtual void push_param_scope() = 0;
    virtual void pop_param_scope()  = 0;
    virtual void push_param_proxy(
        V8BindProxy* bind_proxy
    ) = 0;

    // mixin
    virtual IScriptMixinCore* get_mixin_core() const = 0;
};

//===============================fwd===============================
struct V8BindTemplate;
struct V8BTPrimitive;
struct V8BTEnum;
struct V8BTMapping;

//===============================field & method data===============================
struct V8BTDataField {
    const RTTRType*       field_owner = nullptr;
    const V8BindTemplate* bind_tp     = nullptr;
    const RTTRFieldData*  rttr_data   = nullptr;

    inline void* solve_address(void* obj, const RTTRType* obj_type) const
    {
        void* field_owner_address = obj_type->cast_to_base(field_owner->type_id(), obj);
        return rttr_data->get_address(field_owner_address);
    }
};
struct V8BTDataStaticField {
    const RTTRType*            field_owner = nullptr;
    const V8BindTemplate*      bind_tp     = nullptr;
    const RTTRStaticFieldData* rttr_data   = nullptr;

    inline void* solve_address() const
    {
        return rttr_data->address;
    }
};
struct V8BTDataParam {
    const V8BindTemplate* bind_tp          = nullptr;
    const RTTRParamData*  rttr_data        = nullptr;
    uint32_t              index            = 0;
    bool                  pass_by_ref      = false;
    bool                  appare_in_return = false;
    ERTTRParamFlag        inout_flag       = ERTTRParamFlag::None;
};
struct V8BTDataReturn {
    const V8BindTemplate* bind_tp     = nullptr;
    bool                  pass_by_ref = false;
    bool                  is_void     = false;
};
struct V8BTDataMethod {
    const RTTRType*       method_owner         = nullptr;
    const RTTRMethodData* rttr_data            = nullptr;
    const RTTRMethodData* rttr_data_mixin_impl = nullptr;
    V8BTDataReturn        return_data          = {};
    Vector<V8BTDataParam> params_data          = {};
    uint32_t              params_count         = 0;
    uint32_t              return_count         = 0;

    bool match_param(
        const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack
    ) const;
    void call(
        const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack,
        void*                                          obj,
        const RTTRType*                                obj_type
    ) const;
};
struct V8BTDataStaticMethod {
    const RTTRType*             method_owner = nullptr;
    const RTTRStaticMethodData* rttr_data    = nullptr;
    V8BTDataReturn              return_data  = {};
    Vector<V8BTDataParam>       params_data  = {};
    uint32_t                    params_count = 0;
    uint32_t                    return_count = 0;

    bool match(
        const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack
    ) const;
    void call(
        const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack
    ) const;
};
struct V8BTDataProperty {
    const V8BindTemplate* proxy_bind_tp = nullptr;
    V8BTDataMethod        getter        = {};
    V8BTDataMethod        setter        = {};
};
struct V8BTDataStaticProperty {
    const V8BindTemplate* proxy_bind_tp = nullptr;
    V8BTDataStaticMethod  getter        = {};
    V8BTDataStaticMethod  setter        = {};
};
struct V8BTDataCtor {
    const RTTRCtorData*   rttr_data   = nullptr;
    Vector<V8BTDataParam> params_data = {};

    bool match(
        const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack
    ) const;
    void call(
        const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack,
        void*                                          obj
    ) const;
};

//===============================bind template===============================
struct V8BindTemplate {
    // getter
    inline IV8BindManager* manager() const { return _manager; }

    // convert api
    virtual v8::Local<v8::Value> to_v8(
        void* native_data
    ) const = 0;
    virtual bool to_native(
        void*                native_data,
        v8::Local<v8::Value> v8_value,
        bool                 is_init
    ) const = 0;
    virtual void init_native(
        void* native_data
    ) const = 0; // TODO. 一般来说只会作为内部之用，不需要暴露成 virtual

    // invoke native api
    virtual bool match_param(
        v8::Local<v8::Value> v8_param
    ) const = 0;
    virtual void push_param_native(
        DynamicStack&        stack,
        const V8BTDataParam& param_bind_tp,
        v8::Local<v8::Value> v8_value
    ) const = 0;
    virtual void push_param_native_pure_out(
        DynamicStack&        stack,
        const V8BTDataParam& param_bind_tp
    ) const = 0;
    virtual v8::Local<v8::Value> read_return_native(
        DynamicStack&         stack,
        const V8BTDataReturn& return_bind_tp
    ) const = 0;
    virtual v8::Local<v8::Value> read_return_from_out_param(
        DynamicStack&        stack,
        const V8BTDataParam& param_bind_tp
    ) const = 0;

    // invoke v8 api
    virtual v8::Local<v8::Value> make_param_v8(
        void*                native_data,
        const V8BTDataParam& param_bind_tp
    ) const = 0;

    // field api
    virtual v8::Local<v8::Value> get_field(
        void*                obj,
        const RTTRType*      obj_type,
        const V8BTDataField& field_bind_tp
    ) const = 0;
    virtual void set_field(
        v8::Local<v8::Value> v8_value,
        void*                obj,
        const RTTRType*      obj_type,
        const V8BTDataField& field_bind_tp
    ) const = 0;
    virtual v8::Local<v8::Value> get_static_field(
        const V8BTDataStaticField& field_bind_tp
    ) const = 0;
    virtual void set_static_field(
        v8::Local<v8::Value>       v8_value,
        const V8BTDataStaticField& field_bind_tp
    ) const = 0;

private:
    IV8BindManager* _manager = nullptr;
};
struct V8BTPrimitive final : V8BindTemplate {
    // convert helper
    v8::Local<v8::Value> to_v8(
        void* native_data
    ) const override final;
    bool to_native(
        void*                native_data,
        v8::Local<v8::Value> v8_value,
        bool                 is_init
    ) const override final;
    void init_native(
        void* native_data
    ) const override final;

    // invoke native api
    bool match_param(
        v8::Local<v8::Value> v8_param
    ) const override final;
    void push_param_native(
        DynamicStack&        stack,
        const V8BTDataParam& param_bind_tp,
        v8::Local<v8::Value> v8_value
    ) const override final;
    void push_param_native_pure_out(
        DynamicStack&        stack,
        const V8BTDataParam& param_bind_tp
    ) const override final;
    v8::Local<v8::Value> read_return_native(
        DynamicStack&         stack,
        const V8BTDataReturn& return_bind_tp
    ) const override final;
    v8::Local<v8::Value> read_return_from_out_param(
        DynamicStack&        stack,
        const V8BTDataParam& param_bind_tp
    ) const override final;

    // invoke v8 api
    v8::Local<v8::Value> make_param_v8(
        void*                native_data,
        const V8BTDataParam& param_bind_tp
    ) const override final;

    // field api
    v8::Local<v8::Value> get_field(
        void*                obj,
        const RTTRType*      obj_type,
        const V8BTDataField& field_bind_tp
    ) const override final;
    void set_field(
        v8::Local<v8::Value> v8_value,
        void*                obj,
        const RTTRType*      obj_type,
        const V8BTDataField& field_bind_tp
    ) const override final;
    v8::Local<v8::Value> get_static_field(
        const V8BTDataStaticField& field_bind_tp
    ) const override final;
    void set_static_field(
        v8::Local<v8::Value>       v8_value,
        const V8BTDataStaticField& field_bind_tp
    ) const override final;

private:
    uint32_t    _size      = 0;
    uint32_t    _alignment = 0;
    GUID        _type_id   = {};
    DtorInvoker _dtor      = nullptr;
};
struct V8BTEnum : V8BindTemplate {
    // convert helper
    v8::Local<v8::Value> to_v8(
        void* native_data
    ) const override final;
    bool to_native(
        void*                native_data,
        v8::Local<v8::Value> v8_value,
        bool                 is_init
    ) const override final;
    void init_native(
        void* native_data
    ) const override final;

    // invoke native api
    bool match_param(
        v8::Local<v8::Value> v8_param
    ) const override final;
    void push_param_native(
        DynamicStack&        stack,
        const V8BTDataParam& param_bind_tp,
        v8::Local<v8::Value> v8_value
    ) const override final;
    void push_param_native_pure_out(
        DynamicStack&        stack,
        const V8BTDataParam& param_bind_tp
    ) const override final;
    v8::Local<v8::Value> read_return_native(
        DynamicStack&         stack,
        const V8BTDataReturn& return_bind_tp
    ) const override final;
    v8::Local<v8::Value> read_return_from_out_param(
        DynamicStack&        stack,
        const V8BTDataParam& param_bind_tp
    ) const override final;

    // invoke v8 api
    v8::Local<v8::Value> make_param_v8(
        void*                native_data,
        const V8BTDataParam& param_bind_tp
    ) const override final;

    // field api
    v8::Local<v8::Value> get_field(
        void*                obj,
        const RTTRType*      obj_type,
        const V8BTDataField& field_bind_tp
    ) const override final;
    void set_field(
        v8::Local<v8::Value> v8_value,
        void*                obj,
        const RTTRType*      obj_type,
        const V8BTDataField& field_bind_tp
    ) const override final;
    v8::Local<v8::Value> get_static_field(
        const V8BTDataStaticField& field_bind_tp
    ) const override final;
    void set_static_field(
        v8::Local<v8::Value>       v8_value,
        const V8BTDataStaticField& field_bind_tp
    ) const override final;

private:
    static void _enum_to_string(const ::v8::FunctionCallbackInfo<::v8::Value>& info);
    static void _enum_from_string(const ::v8::FunctionCallbackInfo<::v8::Value>& info);

private:
    const RTTRType*                      _rttr_type  = nullptr;
    V8BTPrimitive*                       _underlying = nullptr;
    Map<String, const RTTREnumItemData*> _items      = {};
    bool                                 _is_signed  = false;
};
struct V8BTMapping : V8BindTemplate {
    // convert helper
    v8::Local<v8::Value> to_v8(
        void* native_data
    ) const override final;
    bool to_native(
        void*                native_data,
        v8::Local<v8::Value> v8_value,
        bool                 is_init
    ) const override final;
    void init_native(
        void* native_data
    ) const override final;

    // invoke native api
    bool match_param(
        v8::Local<v8::Value> v8_param
    ) const override final;
    void push_param_native(
        DynamicStack&        stack,
        const V8BTDataParam& param_bind_tp,
        v8::Local<v8::Value> v8_value
    ) const override final;
    void push_param_native_pure_out(
        DynamicStack&        stack,
        const V8BTDataParam& param_bind_tp
    ) const override final;
    v8::Local<v8::Value> read_return_native(
        DynamicStack&         stack,
        const V8BTDataReturn& return_bind_tp
    ) const override final;
    v8::Local<v8::Value> read_return_from_out_param(
        DynamicStack&        stack,
        const V8BTDataParam& param_bind_tp
    ) const override final;

    // invoke v8 api
    v8::Local<v8::Value> make_param_v8(
        void*                native_data,
        const V8BTDataParam& param_bind_tp
    ) const override final;

    // field api
    v8::Local<v8::Value> get_field(
        void*                obj,
        const RTTRType*      obj_type,
        const V8BTDataField& field_bind_tp
    ) const override final;
    void set_field(
        v8::Local<v8::Value> v8_value,
        void*                obj,
        const RTTRType*      obj_type,
        const V8BTDataField& field_bind_tp
    ) const override final;
    v8::Local<v8::Value> get_static_field(
        const V8BTDataStaticField& field_bind_tp
    ) const override final;
    void set_static_field(
        v8::Local<v8::Value>       v8_value,
        const V8BTDataStaticField& field_bind_tp
    ) const override final;

private:
    const RTTRType*            _rttr_type    = nullptr;
    RTTRInvokerDefaultCtor     _default_ctor = nullptr;
    Map<String, V8BTDataField> _fields       = {};
};
struct V8BTRecordBase : V8BindTemplate {

protected:
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

protected:
    const RTTRType*        _rttr_type         = nullptr;
    bool                   _is_script_newable = false;
    RTTRInvokerDefaultCtor _default_ctor      = nullptr;
    DtorInvoker            _dtor              = nullptr;

    V8BTDataCtor                      _ctor              = {};
    Map<String, V8BTDataField>        _fields            = {};
    Map<String, V8BTDataStaticField>  _static_fields     = {};
    Map<String, V8BTDataMethod>       _methods           = {};
    Map<String, V8BTDataStaticMethod> _static_methods    = {};
    Map<String, V8BTDataProperty>     _properties        = {};
    Map<String, V8BTDataStaticField>  _static_properties = {};
};
struct V8BTValue : V8BTRecordBase {
    // convert helper
    v8::Local<v8::Value> to_v8(
        void* native_data
    ) const override final;
    bool to_native(
        void*                native_data,
        v8::Local<v8::Value> v8_value,
        bool                 is_init
    ) const override final;
    void init_native(
        void* native_data
    ) const override final;

    // invoke native api
    bool match_param(
        v8::Local<v8::Value> v8_param
    ) const override final;
    void push_param_native(
        DynamicStack&        stack,
        const V8BTDataParam& param_bind_tp,
        v8::Local<v8::Value> v8_value
    ) const override final;
    void push_param_native_pure_out(
        DynamicStack&        stack,
        const V8BTDataParam& param_bind_tp
    ) const override final;
    v8::Local<v8::Value> read_return_native(
        DynamicStack&         stack,
        const V8BTDataReturn& return_bind_tp
    ) const override final;
    v8::Local<v8::Value> read_return_from_out_param(
        DynamicStack&        stack,
        const V8BTDataParam& param_bind_tp
    ) const override final;

    // invoke v8 api
    v8::Local<v8::Value> make_param_v8(
        void*                native_data,
        const V8BTDataParam& param_bind_tp
    ) const override final;

    // field api
    v8::Local<v8::Value> get_field(
        void*                obj,
        const RTTRType*      obj_type,
        const V8BTDataField& field_bind_tp
    ) const override final;
    void set_field(
        v8::Local<v8::Value> v8_value,
        void*                obj,
        const RTTRType*      obj_type,
        const V8BTDataField& field_bind_tp
    ) const override final;
    v8::Local<v8::Value> get_static_field(
        const V8BTDataStaticField& field_bind_tp
    ) const override final;
    void set_static_field(
        v8::Local<v8::Value>       v8_value,
        const V8BTDataStaticField& field_bind_tp
    ) const override final;

protected:
    // helper
    V8BPValue* _create_value(void* native_data = nullptr) const;
    V8BPValue* _new_bind_proxy(void* address, v8::Local<v8::Object> self = {}) const;

    // v8 callback
    static void _gc_callback(const ::v8::WeakCallbackInfo<V8BPValue>& data);
    static void _call_ctor(const ::v8::FunctionCallbackInfo<::v8::Value>& info);

private:
    RTTRInvokerAssign                  _assign      = nullptr;
    RTTRInvokerCopyCtor                _copy_ctor   = nullptr;
    v8::Global<::v8::FunctionTemplate> _v8_template = {};
};
struct V8BTObject : V8BTRecordBase {
    // convert helper
    v8::Local<v8::Value> to_v8(
        void* native_data
    ) const override final;
    bool to_native(
        void*                native_data,
        v8::Local<v8::Value> v8_value,
        bool                 is_init
    ) const override final;
    void init_native(
        void* native_data
    ) const override final;

    // invoke native api
    bool match_param(
        v8::Local<v8::Value> v8_param
    ) const override final;
    void push_param_native(
        DynamicStack&        stack,
        const V8BTDataParam& param_bind_tp,
        v8::Local<v8::Value> v8_value
    ) const override final;
    void push_param_native_pure_out(
        DynamicStack&        stack,
        const V8BTDataParam& param_bind_tp
    ) const override final;
    v8::Local<v8::Value> read_return_native(
        DynamicStack&         stack,
        const V8BTDataReturn& return_bind_tp
    ) const override final;
    v8::Local<v8::Value> read_return_from_out_param(
        DynamicStack&        stack,
        const V8BTDataParam& param_bind_tp
    ) const override final;

    // invoke v8 api
    v8::Local<v8::Value> make_param_v8(
        void*                native_data,
        const V8BTDataParam& param_bind_tp
    ) const override final;

    // field api
    v8::Local<v8::Value> get_field(
        void*                obj,
        const RTTRType*      obj_type,
        const V8BTDataField& field_bind_tp
    ) const override final;
    void set_field(
        v8::Local<v8::Value> v8_value,
        void*                obj,
        const RTTRType*      obj_type,
        const V8BTDataField& field_bind_tp
    ) const override final;
    v8::Local<v8::Value> get_static_field(
        const V8BTDataStaticField& field_bind_tp
    ) const override final;
    void set_static_field(
        v8::Local<v8::Value>       v8_value,
        const V8BTDataStaticField& field_bind_tp
    ) const override final;

protected:
    // helper
    V8BPObject* _get_or_make_proxy(void* address) const;
    V8BPObject* _new_bind_proxy(void* address, v8::Local<v8::Object> self = {}) const;

    // v8 callback
    static void _gc_callback(const ::v8::WeakCallbackInfo<V8BPObject>& data);
    static void _call_ctor(const ::v8::FunctionCallbackInfo<::v8::Value>& info);

private:
    v8::Global<::v8::FunctionTemplate> _v8_template = {};
};
} // namespace skr