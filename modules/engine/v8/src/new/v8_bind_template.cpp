#include <SkrV8/new/v8_bind_template.hpp>
#include <SkrV8/v8_bind.hpp>
#include <SkrV8/new/v8_bind_proxy.hpp>

// v8 includes
#include <libplatform/libplatform.h>
#include <v8-initialization.h>
#include <v8-template.h>
#include <v8-external.h>
#include <v8-function.h>
#include <v8-container.h>

// helper
namespace skr::helper
{
inline static bool match_params(
    span<const V8BTDataParam>                      params,
    uint32_t                                       solved_param_count,
    const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack
)
{
    // check param count
    if (solved_param_count != v8_stack.Length()) { return {}; }

    // check param type
    for (size_t i = 0; i < params.size(); i++)
    {
        auto& param_data = params[i];
        auto  v8_arg     = v8_stack[i];

        // skip pure out param
        if (param_data.inout_flag == ERTTRParamFlag::Out) { continue; }

        // do match
        bool matched = param_data.bind_tp->match_param(v8_arg);
        if (!matched) { return false; }
    }

    return true;
}
inline static v8::Local<v8::Value> read_return(
    DynamicStack&             stack,
    span<const V8BTDataParam> params,
    const V8BTDataReturn&     return_bind_tp,
    uint32_t                  solved_return_count
)
{
    auto* isolate = v8::Isolate::GetCurrent();
    auto  context = isolate->GetCurrentContext();

    if (solved_return_count == 1)
    { // return single value
        if (return_bind_tp.is_void)
        { // read from out param
            for (const auto& param_bind_tp : params)
            {
                if (param_bind_tp.appare_in_return)
                {
                    return param_bind_tp.bind_tp->read_return_from_out_param(
                        stack,
                        param_bind_tp
                    );
                }
            }
        }
        else
        { // read return value
            if (stack.is_return_stored())
            {
                return return_bind_tp.bind_tp->read_return_native(
                    stack,
                    return_bind_tp
                );
            }
        }
    }
    else
    { // return param array
        v8::Local<v8::Array> out_array = v8::Array::New(isolate);
        uint32_t             cur_index = 0;

        // try read return value
        if (!return_bind_tp.is_void)
        {
            if (stack.is_return_stored())
            {
                v8::Local<v8::Value> out_value = return_bind_tp.bind_tp->read_return_native(
                    stack,
                    return_bind_tp
                );
                out_array->Set(context, cur_index, out_value).Check();
                ++cur_index;
            }
        }

        // read return value from out param
        for (const auto& param_bind_tp : params)
        {
            if (param_bind_tp.appare_in_return)
            {
                // clang-format off
                out_array->Set(
                    context, 
                    cur_index, 
                param_bind_tp.bind_tp->read_return_from_out_param(
                    stack, 
                    param_bind_tp
                )).Check();
                // clang-format on
                ++cur_index;
            }
        }

        return out_array;
    }

    return {};
}
template <typename T>
inline static T* get_bind_proxy(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    auto  self          = info.This();
    void* external_data = self->GetInternalField(0).As<External>()->Value();
    return reinterpret_cast<T*>(external_data);
}
template <typename T>
inline static T* get_bind_proxy(const v8::Local<v8::Object>& obj)
{
    using namespace ::v8;

    void* external_data = obj->GetInternalField(0).As<External>()->Value();
    return reinterpret_cast<T*>(external_data);
}
template <typename T>
inline static T* get_bind_data(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    void* external_data = info.Data().As<External>()->Value();
    return reinterpret_cast<T*>(external_data);
}
} // namespace skr::helper

namespace skr
{
//===============================field & method data===============================
bool V8BTDataMethod::match_param(
    const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack
) const
{
    return helper::match_params(
        params_data,
        params_count,
        v8_stack
    );
}
void V8BTDataMethod::call(
    const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack,
    void*                                          obj,
    const RTTRType*                                obj_type
) const
{
    DynamicStack native_stack;

    // push param
    uint32_t v8_stack_index = 0;
    for (const auto& param : params_data)
    {
        if (param.inout_flag == ERTTRParamFlag::Out)
        { // pure out param, we will push a dummy xvalue
            param.bind_tp->push_param_native_pure_out(
                native_stack,
                param
            );
        }
        else
        {
            param.bind_tp->push_param_native(
                native_stack,
                param,
                v8_stack[v8_stack_index]
            );
            ++v8_stack_index;
        }
    }

    // get call obj address
    void* owner_address = obj_type->cast_to_base(obj_type->type_id(), obj);

    // invoke
    native_stack.return_behaviour_store();
    if (rttr_data_mixin_impl)
    { // mixin case, invoke minin impl
        rttr_data_mixin_impl->dynamic_stack_invoke(owner_address, native_stack);
    }
    else
    { // normal case
        rttr_data->dynamic_stack_invoke(owner_address, native_stack);
    }

    // read return
    auto return_value = helper::read_return(
        native_stack,
        params_data,
        return_data,
        return_count
    );
    if (!return_value.IsEmpty())
    {
        v8_stack.GetReturnValue().Set(return_value);
    }
}

bool V8BTDataStaticMethod::match(
    const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack
) const
{
    return helper::match_params(
        params_data,
        params_count,
        v8_stack
    );
}
void V8BTDataStaticMethod::call(
    const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack
) const
{
    DynamicStack native_stack;

    // push param
    uint32_t v8_stack_index = 0;
    for (const auto& param : params_data)
    {
        if (param.inout_flag == ERTTRParamFlag::Out)
        { // pure out param, we will push a dummy xvalue
            param.bind_tp->push_param_native_pure_out(
                native_stack,
                param
            );
        }
        else
        {
            param.bind_tp->push_param_native(
                native_stack,
                param,
                v8_stack[v8_stack_index]
            );
            ++v8_stack_index;
        }
    }

    // invoke
    native_stack.return_behaviour_store();
    rttr_data->dynamic_stack_invoke(native_stack);

    // read return
    auto return_value = helper::read_return(
        native_stack,
        params_data,
        return_data,
        return_count
    );
    if (!return_value.IsEmpty())
    {
        v8_stack.GetReturnValue().Set(return_value);
    }
}

bool V8BTDataCtor::match(
    const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack
) const
{
    return helper::match_params(
        params_data,
        params_data.size(),
        v8_stack
    );
}
void V8BTDataCtor::call(
    const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack,
    void*                                          obj
) const
{
    DynamicStack native_stack;

    // push param
    uint32_t v8_stack_index = 0;
    for (const auto& param : params_data)
    {
        param.bind_tp->push_param_native(
            native_stack,
            param,
            v8_stack[v8_stack_index]
        );
        ++v8_stack_index;
    }

    // invoke
    native_stack.return_behaviour_discard();
    rttr_data->dynamic_stack_invoke(obj, native_stack);
}

//======================primitive======================
v8::Local<v8::Value> V8BTPrimitive::to_v8(
    void* native_data
) const
{
    switch (_type_id.get_hash())
    {
    case type_id_of<int8_t>().get_hash():
        return V8Bind::to_v8(*reinterpret_cast<int8_t*>(native_data));
    case type_id_of<int16_t>().get_hash():
        return V8Bind::to_v8(*reinterpret_cast<int16_t*>(native_data));
    case type_id_of<int32_t>().get_hash():
        return V8Bind::to_v8(*reinterpret_cast<int32_t*>(native_data));
    case type_id_of<int64_t>().get_hash():
        return V8Bind::to_v8(*reinterpret_cast<int64_t*>(native_data));
    case type_id_of<uint8_t>().get_hash():
        return V8Bind::to_v8(*reinterpret_cast<uint8_t*>(native_data));
    case type_id_of<uint16_t>().get_hash():
        return V8Bind::to_v8(*reinterpret_cast<uint16_t*>(native_data));
    case type_id_of<uint32_t>().get_hash():
        return V8Bind::to_v8(*reinterpret_cast<uint32_t*>(native_data));
    case type_id_of<uint64_t>().get_hash():
        return V8Bind::to_v8(*reinterpret_cast<uint64_t*>(native_data));
    case type_id_of<float>().get_hash():
        return V8Bind::to_v8(*reinterpret_cast<float*>(native_data));
    case type_id_of<double>().get_hash():
        return V8Bind::to_v8(*reinterpret_cast<double*>(native_data));
    case type_id_of<bool>().get_hash():
        return V8Bind::to_v8(*reinterpret_cast<bool*>(native_data));
    case type_id_of<skr::String>().get_hash():
        return V8Bind::to_v8(*reinterpret_cast<skr::String*>(native_data));
    case type_id_of<skr::StringView>().get_hash():
        return V8Bind::to_v8(*reinterpret_cast<skr::StringView*>(native_data));
    default:
        SKR_UNREACHABLE_CODE()
        return {};
    }
}
bool V8BTPrimitive::to_native(
    void*                native_data,
    v8::Local<v8::Value> v8_value,
    bool                 is_init
) const
{
    if (!is_init)
    {
        init_native(native_data);
    }

    switch (_type_id.get_hash())
    {
    case type_id_of<int8_t>().get_hash():
        return V8Bind::to_native(v8_value, *reinterpret_cast<int8_t*>(native_data));
    case type_id_of<int16_t>().get_hash():
        return V8Bind::to_native(v8_value, *reinterpret_cast<int16_t*>(native_data));
    case type_id_of<int32_t>().get_hash():
        return V8Bind::to_native(v8_value, *reinterpret_cast<int32_t*>(native_data));
    case type_id_of<int64_t>().get_hash():
        return V8Bind::to_native(v8_value, *reinterpret_cast<int64_t*>(native_data));
    case type_id_of<uint8_t>().get_hash():
        return V8Bind::to_native(v8_value, *reinterpret_cast<uint8_t*>(native_data));
    case type_id_of<uint16_t>().get_hash():
        return V8Bind::to_native(v8_value, *reinterpret_cast<uint16_t*>(native_data));
    case type_id_of<uint32_t>().get_hash():
        return V8Bind::to_native(v8_value, *reinterpret_cast<uint32_t*>(native_data));
    case type_id_of<uint64_t>().get_hash():
        return V8Bind::to_native(v8_value, *reinterpret_cast<uint64_t*>(native_data));
    case type_id_of<float>().get_hash():
        return V8Bind::to_native(v8_value, *reinterpret_cast<float*>(native_data));
    case type_id_of<double>().get_hash():
        return V8Bind::to_native(v8_value, *reinterpret_cast<double*>(native_data));
    case type_id_of<bool>().get_hash():
        return V8Bind::to_native(v8_value, *reinterpret_cast<bool*>(native_data));
    case type_id_of<skr::String>().get_hash():
        return V8Bind::to_native(v8_value, *reinterpret_cast<skr::String*>(native_data));
    default:
        SKR_UNREACHABLE_CODE()
        return false;
    }
}
void V8BTPrimitive::init_native(
    void* native_data
) const
{
    switch (_type_id.get_hash())
    {
    case type_id_of<int8_t>().get_hash():
    case type_id_of<int16_t>().get_hash():
    case type_id_of<int32_t>().get_hash():
    case type_id_of<int64_t>().get_hash():
    case type_id_of<uint8_t>().get_hash():
    case type_id_of<uint16_t>().get_hash():
    case type_id_of<uint32_t>().get_hash():
    case type_id_of<uint64_t>().get_hash():
    case type_id_of<float>().get_hash():
    case type_id_of<double>().get_hash():
    case type_id_of<bool>().get_hash():
        return;
    case type_id_of<skr::String>().get_hash():
        new (native_data) skr::String();
        return;
    case type_id_of<skr::StringView>().get_hash():
        new (native_data) skr::StringView();
        return;
    default:
        SKR_UNREACHABLE_CODE()
        return;
    }
}

// invoke native api
bool V8BTPrimitive::match_param(
    v8::Local<v8::Value> v8_param
) const
{
    switch (_type_id.get_hash())
    {
    case type_id_of<int8_t>().get_hash():
        return v8_param->IsInt32();
    case type_id_of<int16_t>().get_hash():
        return v8_param->IsInt32();
    case type_id_of<int32_t>().get_hash():
        return v8_param->IsInt32();
    case type_id_of<int64_t>().get_hash():
        if (v8_param->IsBigInt())
        {
            return true;
        }
        else if (v8_param->IsInt32() || v8_param->IsUint32())
        {
            return true;
        }
        else
        {
            return {};
        }
    case type_id_of<uint8_t>().get_hash():
        return v8_param->IsUint32();
    case type_id_of<uint16_t>().get_hash():
        return v8_param->IsUint32();
    case type_id_of<uint32_t>().get_hash():
        return v8_param->IsUint32();
    case type_id_of<uint64_t>().get_hash():
        if (v8_param->IsBigInt())
        {
            return true;
        }
        else if (v8_param->IsInt32() || v8_param->IsUint32())
        {
            return true;
        }
        else
        {
            return {};
        }
    case type_id_of<float>().get_hash():
        return v8_param->IsNumber();
    case type_id_of<double>().get_hash():
        return v8_param->IsNumber();
    case type_id_of<bool>().get_hash():
        return v8_param->IsBoolean();
    case type_id_of<skr::String>().get_hash():
        return v8_param->IsString();
    case type_id_of<skr::StringView>().get_hash():
        return v8_param->IsString();
    default:
        SKR_UNREACHABLE_CODE()
        return false;
    }
}
void V8BTPrimitive::push_param_native(
    DynamicStack&        stack,
    const V8BTDataParam& param_bind_tp,
    v8::Local<v8::Value> v8_value
) const
{
    struct V8StringViewStackProxy {
        Placeholder<v8::String::Utf8Value> v;
        skr::StringView                    view;

        static void* custom_mapping(void* obj)
        {
            return &reinterpret_cast<V8StringViewStackProxy*>(obj)->view;
        }
        ~V8StringViewStackProxy()
        {
            v.data_typed()->~Utf8Value();
        }
    };

    if (_type_id == type_id_of<StringView>())
    { // string view case, push proxy
        auto* proxy = stack.alloc_param<V8StringViewStackProxy>(
            EDynamicStackParamKind::Direct,
            nullptr,
            V8StringViewStackProxy::custom_mapping
        );
        new (proxy) V8StringViewStackProxy();
        new (proxy->v.data_typed()) v8::String::Utf8Value(v8::Isolate::GetCurrent(), v8_value);
        proxy->view = {
            reinterpret_cast<const skr_char8*>(**proxy->v.data_typed()),
            (uint64_t)proxy->v.data_typed()->length()
        };
    }
    else
    {
        void* native_data = stack.alloc_param_raw(
            _size,
            _alignment,
            param_bind_tp.pass_by_ref ? EDynamicStackParamKind::XValue : EDynamicStackParamKind::Direct,
            _dtor
        );
        to_native(native_data, v8_value, false);
    }
}
void V8BTPrimitive::push_param_native_pure_out(
    DynamicStack&        stack,
    const V8BTDataParam& param_bind_tp
) const
{
    void* native_data = stack.alloc_param_raw(
        _size,
        _alignment,
        param_bind_tp.pass_by_ref ? EDynamicStackParamKind::XValue : EDynamicStackParamKind::Direct,
        _dtor
    );

    // call ctor
    init_native(native_data);
}
v8::Local<v8::Value> V8BTPrimitive::read_return_native(
    DynamicStack&         stack,
    const V8BTDataReturn& return_bind_tp
) const
{
    if (!stack.is_return_stored()) { return {}; }

    void* native_data = stack.get_return_raw();
    if (return_bind_tp.pass_by_ref)
    {
        native_data = *reinterpret_cast<void**>(native_data);
    }
    return to_v8(native_data);
}
v8::Local<v8::Value> V8BTPrimitive::read_return_from_out_param(
    DynamicStack&        stack,
    const V8BTDataParam& param_bind_tp
) const
{
    // get out param data (when pass by ref, we always push xvalue into dynamic stack)
    void* native_data = stack.get_param_raw(param_bind_tp.index);
    return to_v8(native_data);
}

// invoke v8 api
v8::Local<v8::Value> V8BTPrimitive::make_param_v8(
    void*                native_data,
    const V8BTDataParam& param_bind_tp
) const
{
    return to_v8(native_data);
}

// field api
v8::Local<v8::Value> V8BTPrimitive::get_field(
    void*                obj,
    const RTTRType*      obj_type,
    const V8BTDataField& field_bind_tp
) const
{
    void* field_addr = field_bind_tp.solve_address(obj, obj_type);
    return to_v8(field_addr);
}
void V8BTPrimitive::set_field(
    v8::Local<v8::Value> v8_value,
    void*                obj,
    const RTTRType*      obj_type,
    const V8BTDataField& field_bind_tp
) const
{
    void* field_addr = field_bind_tp.solve_address(obj, obj_type);
    to_native(field_addr, v8_value, false);
}
v8::Local<v8::Value> V8BTPrimitive::get_static_field(
    const V8BTDataStaticField& field_bind_tp
) const
{
    void* field_addr = field_bind_tp.solve_address();
    return to_v8(field_addr);
}
void V8BTPrimitive::set_static_field(
    v8::Local<v8::Value>       v8_value,
    const V8BTDataStaticField& field_bind_tp
) const
{
    void* field_addr = field_bind_tp.solve_address();
    to_native(field_addr, v8_value, false);
}

//======================enum======================
v8::Local<v8::Value> V8BTEnum::to_v8(
    void* native_data
) const
{
    return _underlying->to_v8(native_data);
}
bool V8BTEnum::to_native(
    void*                native_data,
    v8::Local<v8::Value> v8_value,
    bool                 is_init
) const
{
    return _underlying->to_native(native_data, v8_value, is_init);
}
void V8BTEnum::init_native(
    void* native_data
) const
{
    return _underlying->init_native(native_data);
}

// invoke native api
bool V8BTEnum::match_param(
    v8::Local<v8::Value> v8_param
) const
{
    return _underlying->match_param(v8_param);
}
void V8BTEnum::push_param_native(
    DynamicStack&        stack,
    const V8BTDataParam& param_bind_tp,
    v8::Local<v8::Value> v8_value
) const
{
    _underlying->push_param_native(
        stack,
        param_bind_tp,
        v8_value
    );
}
void V8BTEnum::push_param_native_pure_out(
    DynamicStack&        stack,
    const V8BTDataParam& param_bind_tp
) const
{
    _underlying->push_param_native_pure_out(
        stack,
        param_bind_tp
    );
}
v8::Local<v8::Value> V8BTEnum::read_return_native(
    DynamicStack&         stack,
    const V8BTDataReturn& return_bind_tp
) const
{
    return _underlying->read_return_native(
        stack,
        return_bind_tp
    );
}
v8::Local<v8::Value> V8BTEnum::read_return_from_out_param(
    DynamicStack&        stack,
    const V8BTDataParam& param_bind_tp
) const
{
    return _underlying->read_return_from_out_param(
        stack,
        param_bind_tp
    );
}

// invoke v8 api
v8::Local<v8::Value> V8BTEnum::make_param_v8(
    void*                native_data,
    const V8BTDataParam& param_bind_tp
) const
{
    return _underlying->make_param_v8(
        native_data,
        param_bind_tp
    );
}

// field api
v8::Local<v8::Value> V8BTEnum::get_field(
    void*                obj,
    const RTTRType*      obj_type,
    const V8BTDataField& field_bind_tp
) const
{
    return _underlying->get_field(
        obj,
        obj_type,
        field_bind_tp
    );
}
void V8BTEnum::set_field(
    v8::Local<v8::Value> v8_value,
    void*                obj,
    const RTTRType*      obj_type,
    const V8BTDataField& field_bind_tp
) const
{
    _underlying->set_field(
        v8_value,
        obj,
        obj_type,
        field_bind_tp
    );
}
v8::Local<v8::Value> V8BTEnum::get_static_field(
    const V8BTDataStaticField& field_bind_tp
) const
{
    return _underlying->get_static_field(field_bind_tp);
}
void V8BTEnum::set_static_field(
    v8::Local<v8::Value>       v8_value,
    const V8BTDataStaticField& field_bind_tp
) const
{
    _underlying->set_static_field(
        v8_value,
        field_bind_tp
    );
}

void V8BTEnum::_enum_to_string(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    Isolate*    Isolate = info.GetIsolate();
    HandleScope HandleScope(Isolate);

    // get user data
    auto* bind_tp = reinterpret_cast<V8BTEnum*>(info.Data().As<External>()->Value());

    // check value
    if (info.Length() != 1)
    {
        Isolate->ThrowError("enum to_string need 1 argument");
        return;
    }

    // get value
    int64_t  enum_singed;
    uint64_t enum_unsigned;
    if (bind_tp->_is_signed)
    {
        if (!V8Bind::to_native(info[0], enum_singed))
        {
            Isolate->ThrowError("invalid enum value");
            return;
        }
    }
    else
    {
        if (!V8Bind::to_native(info[0], enum_unsigned))
        {
            Isolate->ThrowError("invalid enum value");
            return;
        }
    }

    // find value
    for (const auto& [enum_item_name, enum_item] : bind_tp->_items)
    {
        if (enum_item->value.is_signed())
        {
            int64_t v = enum_item->value.value_signed();
            if (v == enum_singed)
            {
                info.GetReturnValue().Set(V8Bind::to_v8(enum_item_name, true));
                return;
            }
        }
        else
        {
            uint64_t v = enum_item->value.value_unsigned();
            if (v == enum_unsigned)
            {
                info.GetReturnValue().Set(V8Bind::to_v8(enum_item_name, true));
                return;
            }
        }
    }

    Isolate->ThrowError("no matched enum item");
}
void V8BTEnum::_enum_from_string(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    Isolate*    Isolate = info.GetIsolate();
    HandleScope HandleScope(Isolate);

    // get user data
    auto* bind_tp = reinterpret_cast<V8BTEnum*>(info.Data().As<External>()->Value());

    // check value
    if (info.Length() != 1)
    {
        Isolate->ThrowError("enum to_string need 1 argument");
        return;
    }

    // get item name
    String enum_item_name;
    if (!V8Bind::to_native(info[0], enum_item_name))
    {
        Isolate->ThrowError("invalid enum item name");
        return;
    }

    // find value
    if (auto result = bind_tp->_items.find(enum_item_name))
    {
        info.GetReturnValue().Set(V8Bind::to_v8(result.value()->value));
    }
    else
    {
        Isolate->ThrowError("no matched enum item");
    }
}

//======================mapping======================
v8::Local<v8::Value> V8BTMapping::to_v8(
    void* native_data
) const
{
    auto isolate = v8::Isolate::GetCurrent();
    auto context = isolate->GetCurrentContext();

    auto result = v8::Object::New(isolate);
    for (const auto& [field_name, field_data] : _fields)
    {
        // conv field to v8
        void* field_addr     = field_data.solve_address(native_data, _rttr_type);
        auto  v8_field_value = field_data.bind_tp->to_v8(field_addr);

        // set object field
        // clang-format off
        result->Set(
            context,
            V8Bind::to_v8(field_name, true),
            v8_field_value
        ).Check();
        // clang-format on
    }
    return result;
}
bool V8BTMapping::to_native(
    void*                native_data,
    v8::Local<v8::Value> v8_value,
    bool                 is_init
) const
{
    auto isolate = v8::Isolate::GetCurrent();
    auto context = isolate->GetCurrentContext();

    auto v8_object = v8_value->ToObject(context).ToLocalChecked();

    // do init
    if (!is_init)
    {
        init_native(native_data);
    }

    // build fields
    for (const auto& [field_name, field_data] : _fields)
    {
        // clang-format off
        // find object field
        auto v8_field = v8_object->Get(
            context,
            V8Bind::to_v8(field_name, true)
        ).ToLocalChecked();
        // clang-format on

        // set field
        void* field_addr = field_data.solve_address(native_data, _rttr_type);
        field_data.bind_tp->to_native(
            field_addr,
            v8_field,
            true
        );
    }
    return true;
}
void V8BTMapping::init_native(
    void* native_data
) const
{
    _default_ctor.invoke(native_data);
}

// invoke native api
bool V8BTMapping::match_param(
    v8::Local<v8::Value> v8_param
) const
{
    auto isolate = v8::Isolate::GetCurrent();
    auto context = isolate->GetCurrentContext();

    // conv to object
    if (!v8_param->IsObject()) { return false; }
    auto v8_obj = v8_param->ToObject(context).ToLocalChecked();

    // match fields
    for (const auto& [field_name, field_data] : _fields)
    {
        // find object field
        auto v8_field_value_maybe = v8_obj->Get(
            context,
            V8Bind::to_v8(field_name, true)
        );
        // clang-format on

        // get field value
        if (v8_field_value_maybe.IsEmpty()) { return {}; }
        auto v8_field_value = v8_field_value_maybe.ToLocalChecked();

        // match field
        auto result = field_data.bind_tp->match_param(v8_field_value);
        if (!result)
        {
            return false;
        }
    }

    return true;
}
void V8BTMapping::push_param_native(
    DynamicStack&        stack,
    const V8BTDataParam& param_bind_tp,
    v8::Local<v8::Value> v8_value
) const
{
    DtorInvoker dtor        = _rttr_type->dtor_invoker();
    void*       native_data = stack.alloc_param_raw(
        _rttr_type->size(),
        _rttr_type->alignment(),
        param_bind_tp.pass_by_ref ? EDynamicStackParamKind::XValue : EDynamicStackParamKind::Direct,
        dtor
    );
    to_native(native_data, v8_value, false);
}
void V8BTMapping::push_param_native_pure_out(
    DynamicStack&        stack,
    const V8BTDataParam& param_bind_tp
) const
{
    DtorInvoker dtor        = _rttr_type->dtor_invoker();
    void*       native_data = stack.alloc_param_raw(
        _rttr_type->size(),
        _rttr_type->alignment(),
        EDynamicStackParamKind::XValue,
        dtor
    );
    init_native(native_data);
}
v8::Local<v8::Value> V8BTMapping::read_return_native(
    DynamicStack&         stack,
    const V8BTDataReturn& return_bind_tp
) const
{
    if (!stack.is_return_stored()) { return {}; }

    void* native_data = stack.get_return_raw();
    if (return_bind_tp.pass_by_ref)
    {
        native_data = *reinterpret_cast<void**>(native_data);
    }
    return to_v8(native_data);
}
v8::Local<v8::Value> V8BTMapping::read_return_from_out_param(
    DynamicStack&        stack,
    const V8BTDataParam& param_bind_tp
) const
{
    // get out param data (when pass by ref, we always push xvalue into dynamic stack)
    void* native_data = stack.get_param_raw(param_bind_tp.index);
    return to_v8(native_data);
}

// invoke v8 api
v8::Local<v8::Value> V8BTMapping::make_param_v8(
    void*                native_data,
    const V8BTDataParam& param_bind_tp
) const
{
    return to_v8(native_data);
}

// field api
v8::Local<v8::Value> V8BTMapping::get_field(
    void*                obj,
    const RTTRType*      obj_type,
    const V8BTDataField& field_bind_tp
) const
{
    void* field_addr = field_bind_tp.solve_address(obj, obj_type);
    return to_v8(field_addr);
}
void V8BTMapping::set_field(
    v8::Local<v8::Value> v8_value,
    void*                obj,
    const RTTRType*      obj_type,
    const V8BTDataField& field_bind_tp
) const
{
    void* field_addr = field_bind_tp.solve_address(obj, obj_type);
    to_native(field_addr, v8_value, false);
}
v8::Local<v8::Value> V8BTMapping::get_static_field(
    const V8BTDataStaticField& field_bind_tp
) const
{
    void* field_addr = field_bind_tp.solve_address();
    return to_v8(field_addr);
}
void V8BTMapping::set_static_field(
    v8::Local<v8::Value>       v8_value,
    const V8BTDataStaticField& field_bind_tp
) const
{
    void* field_addr = field_bind_tp.solve_address();
    to_native(field_addr, v8_value, false);
}

//======================record base======================
void V8BTRecordBase::_call_method(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    Isolate*    Isolate = info.GetIsolate();
    HandleScope HandleScope(Isolate);

    // block ctor call
    if (info.IsConstructCall())
    {
        Isolate->ThrowError("method can not be called with new");
        return;
    }

    // get external data
    auto* bind_proxy = helper::get_bind_proxy<V8BPRecord>(info);
    auto* bind_data  = helper::get_bind_data<V8BTDataMethod>(info);

    // check bind proxy
    if (!bind_proxy->is_valid())
    {
        Isolate->ThrowError("calling on a released object");
        return;
    }

    // match params
    if (!bind_data->match_param(info))
    {
        Isolate->ThrowError("params mismatch");
        return;
    }

    // invoke method
    bind_data->call(
        info,
        bind_proxy->address,
        bind_proxy->rttr_type
    );
}
void V8BTRecordBase::_call_static_method(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    Isolate*    Isolate = info.GetIsolate();
    HandleScope HandleScope(Isolate);

    // block ctor call
    if (info.IsConstructCall())
    {
        Isolate->ThrowError("method can not be called with new");
        return;
    }

    // get external data
    auto* bind_data = helper::get_bind_data<V8BTDataStaticMethod>(info);

    // match params
    if (!bind_data->match(info))
    {
        Isolate->ThrowError("params mismatch");
        return;
    }

    // invoke method
    bind_data->call(
        info
    );
}
void V8BTRecordBase::_get_field(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    Isolate*    Isolate = info.GetIsolate();
    HandleScope HandleScope(Isolate);

    // get external data
    auto* bind_proxy = helper::get_bind_proxy<V8BPRecord>(info);
    auto* bind_data  = helper::get_bind_data<V8BTDataField>(info);

    // check bind proxy
    if (!bind_proxy->is_valid())
    {
        Isolate->ThrowError("calling on a released object");
        return;
    }

    // get field
    auto v8_field = bind_data->bind_tp->get_field(
        bind_proxy->address,
        bind_proxy->rttr_type,
        *bind_data
    );
    info.GetReturnValue().Set(v8_field);
}
void V8BTRecordBase::_set_field(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    Isolate*    Isolate = info.GetIsolate();
    HandleScope HandleScope(Isolate);

    // get external data
    auto* bind_proxy = helper::get_bind_proxy<V8BPRecord>(info);
    auto* bind_data  = helper::get_bind_data<V8BTDataField>(info);

    // check bind proxy
    if (!bind_proxy->is_valid())
    {
        Isolate->ThrowError("calling on a released object");
        return;
    }

    // set field
    bind_data->bind_tp->set_field(
        info[0],
        bind_proxy->address,
        bind_proxy->rttr_type,
        *bind_data
    );
}
void V8BTRecordBase::_get_static_field(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    Isolate*    Isolate = info.GetIsolate();
    HandleScope HandleScope(Isolate);

    // get external data
    auto* bind_data = helper::get_bind_data<V8BTDataStaticField>(info);

    // get field
    auto v8_field = bind_data->bind_tp->get_static_field(
        *bind_data
    );
    info.GetReturnValue().Set(v8_field);
}
void V8BTRecordBase::_set_static_field(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    Isolate*    Isolate = info.GetIsolate();
    HandleScope HandleScope(Isolate);

    // get external data
    auto* bind_data = helper::get_bind_data<V8BTDataStaticField>(info);

    // set field
    bind_data->bind_tp->set_static_field(
        info[0],
        *bind_data
    );
}
void V8BTRecordBase::_get_prop(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    Isolate*    Isolate = info.GetIsolate();
    HandleScope HandleScope(Isolate);

    // get external data
    auto* bind_proxy = helper::get_bind_proxy<V8BPRecord>(info);
    auto* bind_data  = helper::get_bind_data<V8BTDataProperty>(info);

    // check bind proxy
    if (!bind_proxy->is_valid())
    {
        Isolate->ThrowError("calling on a released object");
        return;
    }

    // match property
    if (!bind_data->getter.match_param(info))
    {
        Isolate->ThrowError("property getter params mismatch");
        return;
    }

    // invoke getter
    bind_data->getter.call(
        info,
        bind_proxy->address,
        bind_proxy->rttr_type
    );
}
void V8BTRecordBase::_set_prop(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    Isolate*    Isolate = info.GetIsolate();
    HandleScope HandleScope(Isolate);

    // get external data
    auto* bind_proxy = helper::get_bind_proxy<V8BPRecord>(info);
    auto* bind_data  = helper::get_bind_data<V8BTDataProperty>(info);

    // check bind proxy
    if (!bind_proxy->is_valid())
    {
        Isolate->ThrowError("calling on a released object");
        return;
    }

    // match
    if (!bind_data->setter.match_param(info))
    {
        Isolate->ThrowError("property setter params mismatch");
        return;
    }

    // invoke setter
    bind_data->setter.call(
        info,
        bind_proxy->address,
        bind_proxy->rttr_type
    );
}
void V8BTRecordBase::_get_static_prop(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    Isolate*    Isolate = info.GetIsolate();
    HandleScope HandleScope(Isolate);

    // get external data
    auto* bind_data = helper::get_bind_data<V8BTDataStaticProperty>(info);

    // match
    if (!bind_data->getter.match(info))
    {
        Isolate->ThrowError("property getter params mismatch");
        return;
    }

    // invoke getter
    bind_data->getter.call(
        info
    );
}
void V8BTRecordBase::_set_static_prop(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    Isolate*    Isolate = info.GetIsolate();
    HandleScope HandleScope(Isolate);

    // get external data
    auto* bind_data = helper::get_bind_data<V8BTDataStaticProperty>(info);

    // match
    if (!bind_data->setter.match(info))
    {
        Isolate->ThrowError("property setter params mismatch");
        return;
    }

    // invoke setter
    bind_data->setter.call(
        info
    );
}

//======================Value======================
// convert helper
v8::Local<v8::Value> V8BTValue::to_v8(
    void* native_data
) const
{
    using namespace ::v8;
    Isolate* isolate = Isolate::GetCurrent();

    auto* bind_core = _create_value(native_data);
    return bind_core->v8_object.Get(isolate);
}
bool V8BTValue::to_native(
    void*                native_data,
    v8::Local<v8::Value> v8_value,
    bool                 is_init
) const
{
    using namespace ::v8;
    auto isolate = Isolate::GetCurrent();
    auto context = isolate->GetCurrentContext();

    auto  v8_object  = v8_value->ToObject(context).ToLocalChecked();
    auto* bind_proxy = helper::get_bind_proxy<V8BPValue>(v8_object);

    // check bind proxy
    if (!bind_proxy->is_valid())
    {
        isolate->ThrowError("calling on a released object");
        return false;
    }

    // do cast
    void* casted_ptr = bind_proxy->rttr_type->cast_to_base(_rttr_type->type_id(), bind_proxy->address);

    // assign or copy ctor
    if (is_init)
    {
        if (!_assign)
        {
            isolate->ThrowError("lost assign operator");
            return false;
        }
        _assign.invoke(native_data, casted_ptr);
    }
    else
    {
        if (!_copy_ctor)
        {
            isolate->ThrowError("lost copy ctor");
            return false;
        }
        _copy_ctor.invoke(native_data, casted_ptr);
    }
    return true;
}
void V8BTValue::init_native(
    void* native_data
) const
{
    SKR_UNREACHABLE_CODE();
}

// invoke native api
bool V8BTValue::match_param(
    v8::Local<v8::Value> v8_param
) const
{
    auto isolate = v8::Isolate::GetCurrent();
    auto context = isolate->GetCurrentContext();

    // check v8 value type
    if (!v8_param->IsObject()) { return {}; }
    auto v8_object = v8_param->ToObject(context).ToLocalChecked();

    // check object
    if (v8_object->InternalFieldCount() < 1) { return {}; }
    auto* bind_proxy = helper::get_bind_proxy<V8BPValue>(v8_object);
    if (!bind_proxy->is_valid()) { return {}; }

    // check inherit
    bool base_on = bind_proxy->rttr_type->based_on(
        _rttr_type->type_id()
    );
    return base_on;
}
void V8BTValue::push_param_native(
    DynamicStack&        stack,
    const V8BTDataParam& param_bind_tp,
    v8::Local<v8::Value> v8_value
) const
{
    void* native_data;
    if (param_bind_tp.pass_by_ref)
    { // optimize for ref case
        auto* isolate = v8::Isolate::GetCurrent();
        auto  context = isolate->GetCurrentContext();

        // get bind core
        auto  v8_object  = v8_value->ToObject(context).ToLocalChecked();
        auto* bind_proxy = helper::get_bind_proxy<V8BPValue>(v8_object);

        // check bind core
        if (!bind_proxy->is_valid())
        {
            isolate->ThrowError("calling on a released object");
            return;
        }

        // push pointer
        auto* cast_ptr = bind_proxy->rttr_type->cast_to_base(
            bind_proxy->rttr_type->type_id(),
            bind_proxy->address
        );
        stack.add_param<void*>(cast_ptr);
    }
    else
    {
        DtorInvoker dtor = _rttr_type->dtor_invoker();
        native_data      = stack.alloc_param_raw(
            _rttr_type->size(),
            _rttr_type->alignment(),
            EDynamicStackParamKind::Direct,
            dtor
        );
        to_native(native_data, v8_value, false);
    }
}
void V8BTValue::push_param_native_pure_out(
    DynamicStack&        stack,
    const V8BTDataParam& param_bind_tp
) const
{
    auto* new_value_core = _create_value(nullptr);
    stack.add_param<void*>(new_value_core->address);
}
v8::Local<v8::Value> V8BTValue::read_return_native(
    DynamicStack&         stack,
    const V8BTDataReturn& return_bind_tp
) const
{
    if (!stack.is_return_stored()) { return {}; }

    void* native_data = stack.get_return_raw();
    if (return_bind_tp.pass_by_ref)
    {
        native_data = *reinterpret_cast<void**>(native_data);
    }
    return to_v8(native_data);
}
v8::Local<v8::Value> V8BTValue::read_return_from_out_param(
    DynamicStack&        stack,
    const V8BTDataParam& param_bind_tp
) const
{
    // get out param param
    void* native_data = stack.get_param_raw(param_bind_tp.index);

    // get created value proxy
    native_data            = *reinterpret_cast<void**>(native_data);
    auto  bind_proxy       = manager()->find_bind_proxy(native_data);
    auto* bind_proxy_value = static_cast<V8BPValue*>(bind_proxy);
    return bind_proxy_value->v8_object.Get(v8::Isolate::GetCurrent());
}

// invoke v8 api
v8::Local<v8::Value> V8BTValue::make_param_v8(
    void*                native_data,
    const V8BTDataParam& param_bind_tp
) const
{
    // make bind proxy
    auto* bind_proxy        = _create_value(native_data);
    bind_proxy->address     = native_data;
    bind_proxy->need_delete = false;

    // push param cache
    manager()->push_param_proxy(bind_proxy);

    return bind_proxy->v8_object.Get(v8::Isolate::GetCurrent());
}

// field api
v8::Local<v8::Value> V8BTValue::get_field(
    void*                obj,
    const RTTRType*      obj_type,
    const V8BTDataField& field_bind_tp
) const
{
    // find cache
    void* field_address = field_bind_tp.solve_address(obj, obj_type);
    if (auto found = manager()->find_bind_proxy(field_address))
    {
        auto* bind_proxy = static_cast<V8BPValue*>(found);
        return bind_proxy->v8_object.Get(v8::Isolate::GetCurrent());
    }

    // make bind proxy
    auto* bind_proxy        = _new_bind_proxy(field_address);
    bind_proxy->need_delete = false;

    // setup owner ship
    auto* parent_bind_proxy = manager()->find_bind_proxy(obj);
    bind_proxy->set_parent(parent_bind_proxy);

    return bind_proxy->v8_object.Get(v8::Isolate::GetCurrent());
}
void V8BTValue::set_field(
    v8::Local<v8::Value> v8_value,
    void*                obj,
    const RTTRType*      obj_type,
    const V8BTDataField& field_bind_tp
) const
{
    to_native(
        field_bind_tp.solve_address(obj, obj_type),
        v8_value,
        true
    );
}
v8::Local<v8::Value> V8BTValue::get_static_field(
    const V8BTDataStaticField& field_bind_tp
) const
{
    // find cache
    void* field_address = field_bind_tp.solve_address();
    if (auto found = manager()->find_bind_proxy(field_address))
    {
        auto* bind_proxy = static_cast<V8BPValue*>(found);
        return bind_proxy->v8_object.Get(v8::Isolate::GetCurrent());
    }

    // make bind proxy
    auto* bind_proxy        = _new_bind_proxy(field_address);
    bind_proxy->need_delete = false;
    return bind_proxy->v8_object.Get(v8::Isolate::GetCurrent());
}
void V8BTValue::set_static_field(
    v8::Local<v8::Value>       v8_value,
    const V8BTDataStaticField& field_bind_tp
) const
{
    to_native(
        field_bind_tp.solve_address(),
        v8_value,
        true
    );
}

// helper
V8BPValue* V8BTValue::_create_value(void* native_data) const
{
    using namespace ::v8;

    auto*       isolate = Isolate::GetCurrent();
    HandleScope handle_scope(isolate);

    // construct value
    void* alloc_mem = _rttr_type->alloc();
    if (native_data)
    {
        // copy data
        _copy_ctor.invoke(alloc_mem, native_data);
    }
    else
    {
        // init data
        _default_ctor.invoke(alloc_mem);
    }

    // make bind proxy
    auto* bind_proxy        = _new_bind_proxy(alloc_mem);
    bind_proxy->need_delete = true;

    return bind_proxy;
}
V8BPValue* V8BTValue::_new_bind_proxy(void* address, v8::Local<v8::Object> self) const
{
    using namespace ::v8;

    auto* isolate = Isolate::GetCurrent();
    auto  context = isolate->GetCurrentContext();

    // make v8 object
    Local<ObjectTemplate> instance_template = _v8_template.Get(isolate)->InstanceTemplate();
    Local<Object>         object =
        self.IsEmpty() ?
                    instance_template->NewInstance(context).ToLocalChecked() :
                    self;

    // make bind core
    V8BPValue* bind_proxy = SkrNew<V8BPValue>();
    bind_proxy->rttr_type = _rttr_type;
    bind_proxy->manager   = manager();
    bind_proxy->bind_tp   = this;
    bind_proxy->address   = address;

    // setup gc callback
    bind_proxy->v8_object.SetWeak(
        bind_proxy,
        _gc_callback,
        WeakCallbackType::kInternalFields
    );

    // set internal field
    object->SetInternalField(0, External::New(isolate, bind_proxy));

    // register bind proxy
    manager()->add_bind_proxy(address, bind_proxy);

    return bind_proxy;
}

// v8 callback
void V8BTValue::_gc_callback(const ::v8::WeakCallbackInfo<V8BPValue>& data)
{
    using namespace ::v8;

    auto* bind_proxy = data.GetParameter();

    // do release
    if (bind_proxy->need_delete)
    {
        // call dtor
        if (bind_proxy->bind_tp->_dtor)
        {
            bind_proxy->bind_tp->_dtor(bind_proxy->address);
        }

        // free memory
        bind_proxy->rttr_type->free(bind_proxy->address);
    }

    // unregister bind proxy
    bind_proxy->manager->remove_bind_proxy(bind_proxy->address, bind_proxy);

    // delete bind proxy
    bind_proxy->v8_object.Reset();
    bind_proxy->invalidate();
    SkrDelete(bind_proxy);
}
void V8BTValue::_call_ctor(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    Isolate*    Isolate = info.GetIsolate();
    HandleScope HandleScope(Isolate);

    // get user data
    auto* bind_tp = helper::get_bind_data<V8BTValue>(info);

    // block not construct call
    if (!info.IsConstructCall())
    {
        Isolate->ThrowError("ctor can only be called with new");
        return;
    }

    // check constructable
    if (!bind_tp->_is_script_newable)
    {
        Isolate->ThrowError("object is not constructable");
        return;
    }

    // check params
    if (!bind_tp->_ctor.match(info))
    {
        Isolate->ThrowError("ctor params mismatch");
        return;
    }

    // new object
    void* alloc_mem = bind_tp->_rttr_type->alloc();
    bind_tp->_ctor.call(info, alloc_mem);

    // make bind proxy
    bind_tp->_new_bind_proxy(alloc_mem, info.This());
}

//======================Object======================
// convert helper
v8::Local<v8::Value> V8BTObject::to_v8(
    void* native_data
) const
{
    using namespace ::v8;

    // get address
    auto* address = *reinterpret_cast<void**>(native_data);

    // get proxy
    auto* bind_proxy = _get_or_make_proxy(address);

    return bind_proxy->v8_object.Get(Isolate::GetCurrent());
}
bool V8BTObject::to_native(
    void*                native_data,
    v8::Local<v8::Value> v8_value,
    bool                 is_init
) const
{
    using namespace ::v8;

    auto isolate = v8::Isolate::GetCurrent();
    auto context = isolate->GetCurrentContext();

    // get bind proxy
    auto  v8_object  = v8_value->ToObject(context).ToLocalChecked();
    auto* bind_proxy = helper::get_bind_proxy<V8BPObject>(v8_object);

    // check bind proxy
    if (!bind_proxy->is_valid())
    {
        isolate->ThrowError("calling on a released object");
        return false;
    }

    // do cast
    void* casted_ptr = bind_proxy->rttr_type->cast_to_base(
        bind_proxy->rttr_type->type_id(),
        bind_proxy->address
    );
    *reinterpret_cast<void**>(native_data) = casted_ptr;

    return true;
}
void V8BTObject::init_native(
    void* native_data
) const
{
    SKR_UNREACHABLE_CODE();
}

// invoke native api
bool V8BTObject::match_param(
    v8::Local<v8::Value> v8_param
) const
{
    auto isolate = v8::Isolate::GetCurrent();
    auto context = isolate->GetCurrentContext();

    // check v8 value type
    if (!v8_param->IsObject()) { return false; }
    auto v8_object = v8_param->ToObject(context).ToLocalChecked();

    // check object internal field
    if (v8_object->InternalFieldCount() < 1) { return false; }

    // check bind proxy
    auto* bind_proxy = helper::get_bind_proxy<V8BPObject>(v8_object);
    if (!bind_proxy->is_valid()) { return false; }

    // check inherit
    bool base_on = bind_proxy->rttr_type->based_on(
        _rttr_type->type_id()
    );
    return base_on;
}
void V8BTObject::push_param_native(
    DynamicStack&        stack,
    const V8BTDataParam& param_bind_tp,
    v8::Local<v8::Value> v8_value
) const
{
    void* native_data = stack.alloc_param_raw(
        sizeof(void*),
        alignof(void*),
        EDynamicStackParamKind::Direct,
        nullptr
    );
    to_native(native_data, v8_value, false);
}
void V8BTObject::push_param_native_pure_out(
    DynamicStack&        stack,
    const V8BTDataParam& param_bind_tp
) const
{
    SKR_UNREACHABLE_CODE();
}
v8::Local<v8::Value> V8BTObject::read_return_native(
    DynamicStack&         stack,
    const V8BTDataReturn& return_bind_tp
) const
{
    if (!stack.is_return_stored()) { return {}; }
    void* native_data = stack.get_return_raw(); // always void**
    return to_v8(native_data);
}
v8::Local<v8::Value> V8BTObject::read_return_from_out_param(
    DynamicStack&        stack,
    const V8BTDataParam& param_bind_tp
) const
{
    void* native_data = stack.get_param_raw(param_bind_tp.index); // always void**
    return to_v8(native_data);
}

// invoke v8 api
v8::Local<v8::Value> V8BTObject::make_param_v8(
    void*                native_data,
    const V8BTDataParam& param_bind_tp
) const
{
    return to_v8(native_data);
}

// field api
v8::Local<v8::Value> V8BTObject::get_field(
    void*                obj,
    const RTTRType*      obj_type,
    const V8BTDataField& field_bind_tp
) const
{
    void* field_address = field_bind_tp.solve_address(obj, obj_type);
    return to_v8(field_address);
}
void V8BTObject::set_field(
    v8::Local<v8::Value> v8_value,
    void*                obj,
    const RTTRType*      obj_type,
    const V8BTDataField& field_bind_tp
) const
{
    void* field_address = field_bind_tp.solve_address(obj, obj_type);
    to_native(field_address, v8_value, false);
}
v8::Local<v8::Value> V8BTObject::get_static_field(
    const V8BTDataStaticField& field_bind_tp
) const
{
    void* field_address = field_bind_tp.solve_address();
    return to_v8(field_address);
}
void V8BTObject::set_static_field(
    v8::Local<v8::Value>       v8_value,
    const V8BTDataStaticField& field_bind_tp
) const
{
    void* field_address = field_bind_tp.solve_address();
    to_native(field_address, v8_value, false);
}

// helper
V8BPObject* V8BTObject::_get_or_make_proxy(void* address) const
{
    using namespace ::v8;

    // find exist bind proxy
    if (auto found = manager()->find_bind_proxy(address))
    {
        return static_cast<V8BPObject*>(found);
    }

    return _new_bind_proxy(address, v8::Local<v8::Object>());
}
V8BPObject* V8BTObject::_new_bind_proxy(void* address, v8::Local<v8::Object> self) const
{
    using namespace ::v8;

    auto*       isolate = Isolate::GetCurrent();
    auto        context = isolate->GetCurrentContext();
    HandleScope handle_scope(isolate);

    // get scriptble object
    ScriptbleObject* scriptble_object;
    {
        void* casted_ptr = _rttr_type->cast_to_base(
            type_id_of<ScriptbleObject>(),
            address
        );
        scriptble_object = reinterpret_cast<ScriptbleObject*>(casted_ptr);
    }

    // make object
    Local<ObjectTemplate> instance_template = _v8_template.Get(isolate)->InstanceTemplate();
    Local<Object>         object =
        self.IsEmpty() ?
                    instance_template->NewInstance(context).ToLocalChecked() :
                    self;

    // make bind proxy
    V8BPObject* bind_proxy = SkrNew<V8BPObject>();
    bind_proxy->rttr_type  = _rttr_type;
    bind_proxy->manager    = manager();
    bind_proxy->bind_tp    = this;
    bind_proxy->address    = address;
    bind_proxy->object     = scriptble_object;

    // setup gc callback
    bind_proxy->v8_object.SetWeak(
        bind_proxy,
        _gc_callback,
        WeakCallbackType::kInternalFields
    );

    // set internal field
    object->SetInternalField(0, ::v8::External::New(isolate, bind_proxy));

    // register bind proxy
    manager()->add_bind_proxy(address, bind_proxy);

    // bind mixin core
    scriptble_object->set_mixin_core(manager()->get_mixin_core());

    return bind_proxy;
}

// v8 callback
void V8BTObject::_gc_callback(const ::v8::WeakCallbackInfo<V8BPObject>& data)
{
    using namespace ::v8;

    auto* bind_proxy = data.GetParameter();

    // do release
    if (bind_proxy->is_valid())
    {
        // remove mixin first, for prevent delete callback
        bind_proxy->object->set_mixin_core(nullptr);

        // release object
        if (bind_proxy->object->ownership() == EScriptbleObjectOwnership::Script)
        {
            SkrDelete(bind_proxy->object);
        }
    }

    // delete bind proxy
    bind_proxy->v8_object.Reset();
    bind_proxy->invalidate();
    SkrDelete(bind_proxy);
}
void V8BTObject::_call_ctor(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    Isolate*    Isolate = info.GetIsolate();
    HandleScope HandleScope(Isolate);

    // get user data
    auto* bind_tp = helper::get_bind_data<V8BTObject>(info);

    // block not construct call
    if (!info.IsConstructCall())
    {
        Isolate->ThrowError("ctor can only be called with new");
        return;
    }

    // check constructable
    if (!bind_tp->_is_script_newable)
    {
        Isolate->ThrowError("object is not constructable");
        return;
    }

    // check params
    if (!bind_tp->_ctor.match(info))
    {
        Isolate->ThrowError("ctor params mismatch");
        return;
    }

    // new object
    void* alloc_mem = bind_tp->_rttr_type->alloc();
    bind_tp->_ctor.call(info, alloc_mem);

    // make bind proxy
    bind_tp->_new_bind_proxy(alloc_mem, info.This());
}

} // namespace skr