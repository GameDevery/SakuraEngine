#include <SkrV8/bind_template/v8bt_primitive.hpp>
#include <SkrV8/v8_bind.hpp>

namespace skr
{
V8BTPrimitive* V8BTPrimitive::TryCreate(IV8BindManager* manager, GUID type_id)
{
    switch (type_id.get_hash())
    {
    case type_id_of<void>().get_hash():
        return _make<void>(manager);
    case type_id_of<int8_t>().get_hash():
        return _make<int8_t>(manager);
    case type_id_of<int16_t>().get_hash():
        return _make<int16_t>(manager);
    case type_id_of<int32_t>().get_hash():
        return _make<int32_t>(manager);
    case type_id_of<int64_t>().get_hash():
        return _make<int64_t>(manager);
    case type_id_of<uint8_t>().get_hash():
        return _make<uint8_t>(manager);
    case type_id_of<uint16_t>().get_hash():
        return _make<uint16_t>(manager);
    case type_id_of<uint32_t>().get_hash():
        return _make<uint32_t>(manager);
    case type_id_of<uint64_t>().get_hash():
        return _make<uint64_t>(manager);
    case type_id_of<float>().get_hash():
        return _make<float>(manager);
    case type_id_of<double>().get_hash():
        return _make<double>(manager);
    case type_id_of<bool>().get_hash():
        return _make<bool>(manager);
    case type_id_of<String>().get_hash():
        return _make<String>(manager);
    case type_id_of<StringView>().get_hash():
        return _make<StringView>(manager);
    default:
        return nullptr;
    }
}

// kind
EV8BTKind V8BTPrimitive::kind() const
{
    return EV8BTKind::Primitive;
}

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
        _init_native(native_data);
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
    _init_native(native_data);
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

void V8BTPrimitive::_init_native(
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
} // namespace skr