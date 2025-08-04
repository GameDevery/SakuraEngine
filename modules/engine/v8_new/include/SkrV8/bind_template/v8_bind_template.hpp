#pragma once
#include <SkrCore/error_collector.hpp>
#include <SkrRTTR/script/scriptble_object.hpp>

// v8 includes
#include <v8-persistent-handle.h>
#include <v8-template.h>
#include <v8-external.h>

namespace skr
{
//===============================fwd===============================
struct V8BindTemplate;
struct V8BTPrimitive;
struct V8BTEnum;
struct V8BTMapping;
struct V8BTGeneric;

struct V8BindProxy;
struct V8BPValue;
struct V8BPObject;

struct IV8BindManager {
    virtual ~IV8BindManager() = default;

    // bind proxy management
    virtual V8BindTemplate* solve_bind_tp(
        TypeSignatureView signature
    ) = 0;
    virtual V8BindTemplate* solve_bind_tp(
        const GUID& type_id
    ) = 0;

    // bind proxy management(template version)
    template <typename T>
    T* solve_bind_tp_as(const GUID& type_id);
    template <typename T>
    T* solve_bind_tp_as(TypeSignatureView type_id);

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

    // get logger
    virtual ErrorCollector& logger() = 0;
};

//===============================field & method data===============================
struct V8BTDataModifier {
    bool is_pointer : 1    = false;
    bool is_ref : 1        = false;
    bool is_rvalue_ref : 1 = false;
    bool is_const : 1      = false;

    inline void solve(TypeSignatureView signature)
    {
        is_pointer    = signature.is_pointer();
        is_ref        = signature.is_ref();
        is_rvalue_ref = signature.is_rvalue_ref();
        is_const      = signature.is_const();
    }
    inline bool is_any_ref() const
    {
        return is_ref || is_rvalue_ref;
    }
    inline bool is_decayed_pointer() const
    {
        return is_pointer || is_any_ref();
    }
};
struct V8BTDataField {
    const RTTRType*       field_owner = nullptr;
    const V8BindTemplate* bind_tp     = nullptr;
    const RTTRFieldData*  rttr_data   = nullptr;
    V8BTDataModifier      modifiers   = {};

    inline void* solve_address(void* obj, const RTTRType* obj_type) const
    {
        void* field_owner_address = obj_type->cast_to_base(field_owner->type_id(), obj);
        return rttr_data->get_address(field_owner_address);
    }
    void setup(
        IV8BindManager*      manager,
        const RTTRFieldData* field_data,
        const RTTRType*      owner
    );
};
struct V8BTDataStaticField {
    const RTTRType*            field_owner = nullptr;
    const V8BindTemplate*      bind_tp     = nullptr;
    const RTTRStaticFieldData* rttr_data   = nullptr;
    V8BTDataModifier           modifiers   = {};

    inline void* solve_address() const
    {
        return rttr_data->address;
    }
    void setup(
        IV8BindManager*            manager,
        const RTTRStaticFieldData* field_data,
        const RTTRType*            owner
    );
};
struct V8BTDataParam {
    const V8BindTemplate* bind_tp   = nullptr;
    const RTTRParamData*  rttr_data = nullptr;
    uint32_t              index     = 0;

    // modifier data
    V8BTDataModifier modifiers  = {};
    ERTTRParamFlag   inout_flag = ERTTRParamFlag::None;

    // solve data
    bool appare_in_return = false;
    bool appare_in_param  = false;

    void setup(
        IV8BindManager*      manager,
        const RTTRParamData* param_data
    );
    void setup(
        IV8BindManager*   manager,
        const StackProxy* proxy,
        int32_t           index
    );
};
struct V8BTDataReturn {
    const V8BindTemplate* bind_tp     = nullptr;
    bool                  pass_by_ref = false;

    // modifier data
    V8BTDataModifier modifiers = {};

    // solve data
    bool is_void = false;

    void setup(
        IV8BindManager*   manager,
        TypeSignatureView signature
    );
};
struct V8BTDataMethod {
    const RTTRType*       method_owner         = nullptr;
    const RTTRMethodData* rttr_data            = nullptr;
    const RTTRMethodData* rttr_data_mixin_impl = nullptr;
    V8BTDataReturn        return_data          = {};
    Vector<V8BTDataParam> params_data          = {};
    uint32_t              params_count         = 0;
    uint32_t              return_count         = 0;

    inline bool is_valid() const
    {
        return rttr_data != nullptr;
    }

    bool match_param(
        const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack
    ) const;
    void call(
        const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack,
        void*                                          obj,
        const RTTRType*                                obj_type
    ) const;
    void setup(
        IV8BindManager*       manager,
        const RTTRMethodData* method_data,
        const RTTRType*       owner
    );
};
struct V8BTDataStaticMethod {
    const RTTRType*             method_owner = nullptr;
    const RTTRStaticMethodData* rttr_data    = nullptr;
    V8BTDataReturn              return_data  = {};
    Vector<V8BTDataParam>       params_data  = {};
    uint32_t                    params_count = 0;
    uint32_t                    return_count = 0;

    inline bool is_valid() const
    {
        return rttr_data != nullptr;
    }

    bool match(
        const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack
    ) const;
    void call(
        const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack
    ) const;
    void setup(
        IV8BindManager*             manager,
        const RTTRStaticMethodData* method_data,
        const RTTRType*             owner
    );
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

    inline bool is_valid() const
    {
        return rttr_data != nullptr;
    }

    bool match(
        const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack
    ) const;
    void call(
        const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack,
        void*                                          obj
    ) const;
    void setup(
        IV8BindManager*     manager,
        const RTTRCtorData* ctor_data
    );
};
struct V8BTDataCallScript {
    V8BTDataReturn        return_data  = {};
    Vector<V8BTDataParam> params_data  = {};
    uint32_t              params_count = 0;
    uint32_t              return_count = 0;

    bool read_return(
        span<const StackProxy>    params,
        StackProxy                return_value,
        v8::MaybeLocal<v8::Value> v8_return_value
    );
    void setup(
        IV8BindManager*        manager,
        span<const StackProxy> params,
        StackProxy             return_value
    );
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

    // basic info
    virtual EV8BTKind kind() const          = 0;
    virtual String    type_name() const     = 0;
    virtual String    cpp_namespace() const = 0;

    // convert api
    virtual v8::Local<v8::Value> to_v8(
        void* native_data
    ) const = 0;
    virtual bool to_native(
        void*                native_data,
        v8::Local<v8::Value> v8_value,
        bool                 is_init
    ) const = 0;

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

    // check api
    // TODO. API 分型, solve 归 solve, check 归 check
    virtual bool solve_param(
        V8BTDataParam& param_bind_tp
    ) const = 0;
    virtual bool solve_return(
        V8BTDataReturn& return_bind_tp
    ) const = 0;
    virtual bool solve_field(
        V8BTDataField& field_bind_tp
    ) const = 0;
    virtual bool solve_static_field(
        V8BTDataStaticField& field_bind_tp
    ) const = 0;

    // v8 export
    virtual bool has_v8_export_obj(
    ) const = 0;
    virtual v8::Local<v8::Value> get_v8_export_obj(
    ) const = 0;

    // cast helper
    template <typename T>
    inline bool is() const
    {
        return kind() == T::kKind;
    }
    template <typename T>
    inline T* as()
    {
        SKR_ASSERT(is<T>());
        return static_cast<T*>(this);
    }
    template <typename T>
    inline const T* as() const
    {
        SKR_ASSERT(is<T>());
        return static_cast<const T*>(this);
    }

private:
    IV8BindManager* _manager = nullptr;
};

template <typename T>
inline T* IV8BindManager::solve_bind_tp_as(const GUID& type_id)
{
    return solve_bind_tp(type_id)->as<T>();
}

template <typename T>
inline T* IV8BindManager::solve_bind_tp_as(TypeSignatureView type_id)
{
    return solve_bind_tp(type_id)->as<T>();
}

// help functions
template <typename T>
inline T* get_bind_proxy(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    auto  self          = info.This();
    void* external_data = self->GetInternalField(0).As<External>()->Value();
    return reinterpret_cast<T*>(external_data);
}
template <typename T>
inline T* get_bind_proxy(const v8::Local<v8::Object>& obj)
{
    using namespace ::v8;

    void* external_data = obj->GetInternalField(0).As<External>()->Value();
    return reinterpret_cast<T*>(external_data);
}
template <typename T>
inline T* get_bind_data(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    void* external_data = info.Data().As<External>()->Value();
    return reinterpret_cast<T*>(external_data);
}
} // namespace skr