#pragma once
#include <SkrRTTR/script/scriptble_object.hpp>

// v8 includes
#include <v8-persistent-handle.h>
#include <v8-template.h>

// temp include
// TODO. remove it
#include "SkrRTTR/script/script_binder.hpp"

namespace skr
{
//===============================fwd===============================
struct V8BindTemplate;
struct V8BTPrimitive;
struct V8BTEnum;
struct V8BTMapping;

struct V8BindProxy;
struct V8BPValue;
struct V8BPObject;

struct IV8BindManager {
    virtual ~IV8BindManager() = default;

    // bind proxy management
    virtual V8BindTemplate* find_or_add_bind_template(
        TypeSignatureView signature
    ) = 0;
    virtual V8BindTemplate* find_or_add_bind_template(
        const GUID& type_id
    ) = 0;

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

    inline void setup(IV8BindManager* manager, const ScriptBinderField& binder)
    {
        field_owner = binder.owner;
        bind_tp     = manager->find_or_add_bind_template(binder.binder.type_id());
        rttr_data   = binder.data;
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
    inline void setup(IV8BindManager* manager, const ScriptBinderStaticField& binder)
    {
        field_owner = binder.owner;
        bind_tp     = manager->find_or_add_bind_template(binder.binder.type_id());
        rttr_data   = binder.data;
    }
};
struct V8BTDataParam {
    const V8BindTemplate* bind_tp          = nullptr;
    const RTTRParamData*  rttr_data        = nullptr;
    uint32_t              index            = 0;
    bool                  pass_by_ref      = false;
    bool                  appare_in_return = false;
    ERTTRParamFlag        inout_flag       = ERTTRParamFlag::None;

    inline void setup(IV8BindManager* manager, const ScriptBinderParam& binder)
    {
        bind_tp          = manager->find_or_add_bind_template(binder.binder.type_id());
        rttr_data        = binder.data;
        index            = binder.index;
        pass_by_ref      = binder.pass_by_ref;
        appare_in_return = binder.appare_in_return;
        inout_flag       = binder.inout_flag;
    }
};
struct V8BTDataReturn {
    const V8BindTemplate* bind_tp     = nullptr;
    bool                  pass_by_ref = false;
    bool                  is_void     = false;

    inline void setup(IV8BindManager* manager, const ScriptBinderReturn& binder)
    {
        bind_tp     = manager->find_or_add_bind_template(binder.binder.type_id());
        pass_by_ref = binder.pass_by_ref;
        is_void     = binder.is_void;
    }
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

    inline void setup(IV8BindManager* manager, const ScriptBinderMethod& binder)
    {
        method_owner         = binder.owner;
        rttr_data            = binder.data;
        rttr_data_mixin_impl = binder.mixin_impl_data;
        return_data.setup(manager, binder.return_binder);
        params_data.resize_default(binder.params_count);
        for (uint32_t i = 0; i < binder.params_count; ++i)
        {
            params_data[i].setup(manager, binder.params_binder[i]);
        }
        params_count = binder.params_count;
        return_count = binder.return_count;
    }
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

    inline void setup(IV8BindManager* manager, const ScriptBinderStaticMethod& binder)
    {
        method_owner = binder.owner;
        rttr_data    = binder.data;
        return_data.setup(manager, binder.return_binder);
        params_data.resize_default(binder.params_count);
        for (uint32_t i = 0; i < binder.params_count; ++i)
        {
            params_data[i].setup(manager, binder.params_binder[i]);
        }
        params_count = binder.params_count;
        return_count = binder.return_count;
    }
};
struct V8BTDataProperty {
    const V8BindTemplate* proxy_bind_tp = nullptr;
    V8BTDataMethod        getter        = {};
    V8BTDataMethod        setter        = {};

    inline void setup(IV8BindManager* manager, const ScriptBinderProperty& binder)
    {
        proxy_bind_tp = manager->find_or_add_bind_template(binder.binder.type_id());
        getter.setup(manager, binder.setter);
        setter.setup(manager, binder.getter);
    }
};
struct V8BTDataStaticProperty {
    const V8BindTemplate* proxy_bind_tp = nullptr;
    V8BTDataStaticMethod  getter        = {};
    V8BTDataStaticMethod  setter        = {};

    inline void setup(IV8BindManager* manager, const ScriptBinderStaticProperty& binder)
    {
        proxy_bind_tp = manager->find_or_add_bind_template(binder.binder.type_id());
        getter.setup(manager, binder.setter);
        setter.setup(manager, binder.getter);
    }
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

    inline void setup(IV8BindManager* manager, const ScriptBinderCtor& binder)
    {
        rttr_data = binder.data;
        params_data.resize_default(binder.params_binder.size());
        for (uint32_t i = 0; i < binder.params_binder.size(); ++i)
        {
            params_data[i].setup(manager, binder.params_binder[i]);
        }
    }
};

//===============================bind template===============================
enum class EV8BTKind
{
    Primitive,
    Enum,
    Mapping,
    Value,
    Object,
    Generic,
};

struct V8BindTemplate {
    // getter & setter
    inline IV8BindManager* manager() const { return _manager; }
    inline void            set_manager(IV8BindManager* manager) { _manager = manager; }

    // kind
    virtual EV8BTKind kind() const = 0;

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
    // kind
    EV8BTKind kind() const override;

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

    void setup(const ScriptBinderPrimitive& binder);

private:
    uint32_t    _size      = 0;
    uint32_t    _alignment = 0;
    GUID        _type_id   = {};
    DtorInvoker _dtor      = nullptr;
};
struct V8BTEnum : V8BindTemplate {
    // kind
    EV8BTKind kind() const override;

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

    void setup(const ScriptBinderEnum& binder);

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
    // kind
    EV8BTKind kind() const override;

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

    void setup(const ScriptBinderMapping& binder);

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

    void _setup(const ScriptBinderRecordBase& binder);

protected:
    const RTTRType*        _rttr_type         = nullptr;
    bool                   _is_script_newable = false;
    RTTRInvokerDefaultCtor _default_ctor      = nullptr;
    DtorInvoker            _dtor              = nullptr;

    V8BTDataCtor                        _ctor              = {};
    Map<String, V8BTDataField>          _fields            = {};
    Map<String, V8BTDataStaticField>    _static_fields     = {};
    Map<String, V8BTDataMethod>         _methods           = {};
    Map<String, V8BTDataStaticMethod>   _static_methods    = {};
    Map<String, V8BTDataProperty>       _properties        = {};
    Map<String, V8BTDataStaticProperty> _static_properties = {};
};
struct V8BTValue : V8BTRecordBase {
    // kind
    EV8BTKind kind() const override;

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

    void setup(const ScriptBinderValue& binder);

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
    // kind
    EV8BTKind kind() const override;

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

    void setup(const ScriptBinderObject& binder);

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