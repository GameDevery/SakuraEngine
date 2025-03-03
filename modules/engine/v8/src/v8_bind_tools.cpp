#include "SkrV8/v8_bind_tools.hpp"

// param helpers
namespace skr::v8
{
// write primitive
inline static bool _push_param_primitive(
    rttr::DynamicStack&          stack,
    skr::GUID                    type_id,
    rttr::EDynamicStackParamKind kind,
    ::v8::Local<::v8::Value>     v8_value,
    ::v8::Local<::v8::Context>   context,
    ::v8::Isolate*               isolate
)
{
    switch (type_id.get_hash())
    {
        // int32/uint32
        case rttr::type_id_of<int8_t>().get_hash():
            stack.add_param<int8_t>((int8_t)v8_value->Int32Value(context).ToChecked(), kind);
            return true;
        case rttr::type_id_of<int16_t>().get_hash():
            stack.add_param<int16_t>((int16_t)v8_value->Int32Value(context).ToChecked(), kind);
            return true;
        case rttr::type_id_of<int32_t>().get_hash():
            stack.add_param<int32_t>((int32_t)v8_value->Int32Value(context).ToChecked(), kind);
            return true;
        case rttr::type_id_of<uint8_t>().get_hash():
            stack.add_param<uint8_t>((uint8_t)v8_value->Uint32Value(context).ToChecked(), kind);
            return true;
        case rttr::type_id_of<uint16_t>().get_hash():
            stack.add_param<uint16_t>((uint16_t)v8_value->Uint32Value(context).ToChecked(), kind);
            return true;
        case rttr::type_id_of<uint32_t>().get_hash():
            stack.add_param<uint32_t>((uint32_t)v8_value->Uint32Value(context).ToChecked(), kind);
            return true;
        // int64/uint64
        case rttr::type_id_of<int64_t>().get_hash():
            stack.add_param<int64_t>((int64_t)v8_value->ToBigInt(context).ToLocalChecked()->Int64Value(), kind);
            return true;
        case rttr::type_id_of<uint64_t>().get_hash():
            stack.add_param<uint64_t>((uint64_t)v8_value->ToBigInt(context).ToLocalChecked()->Uint64Value(), kind);
            return true;
        // float/double
        case rttr::type_id_of<float>().get_hash():
            stack.add_param<float>((float)v8_value->NumberValue(context).ToChecked(), kind);
            return true;
        case rttr::type_id_of<double>().get_hash():
            stack.add_param<double>((double)v8_value->NumberValue(context).ToChecked(), kind);
            return true;
        // bool
        case rttr::type_id_of<bool>().get_hash():
            stack.add_param<bool>(v8_value->BooleanValue(isolate), kind);
            return true;
        // string
        case rttr::type_id_of<skr::String>().get_hash():
            stack.add_param<skr::String>(skr::String::From(*::v8::String::Utf8Value(isolate, v8_value)), kind);
            return true;
        default:
            return false;
    }
}
}

namespace skr::v8
{
// match tools
bool V8BindTools::match_type(::v8::Local<::v8::Value> v8_value, rttr::TypeSignatureView native_signature)
{
    if (native_signature.is_type())
    {
        // read type id
        native_signature.jump_modifier();
        GUID type_id;
        native_signature.read_type_id(type_id);

        // match primitive types
        switch (type_id.get_hash())
        {
            case rttr::type_id_of<int8_t>().get_hash():
                return v8_value->IsInt32();
            case rttr::type_id_of<int16_t>().get_hash():
                return v8_value->IsInt32();
            case rttr::type_id_of<int32_t>().get_hash():
                return v8_value->IsInt32();
            case rttr::type_id_of<uint8_t>().get_hash():
                return v8_value->IsUint32();
            case rttr::type_id_of<uint16_t>().get_hash():
                return v8_value->IsUint32();
            case rttr::type_id_of<uint32_t>().get_hash():
                return v8_value->IsUint32();
            case rttr::type_id_of<int64_t>().get_hash():
                return v8_value->IsBigInt();
            case rttr::type_id_of<uint64_t>().get_hash():
                return v8_value->IsBigInt();
            case rttr::type_id_of<float>().get_hash():
                return v8_value->IsNumber();
            case rttr::type_id_of<double>().get_hash():
                return v8_value->IsNumber();
            case rttr::type_id_of<bool>().get_hash():
                return v8_value->IsBoolean();
            case rttr::type_id_of<skr::String>().get_hash():
                return v8_value->IsString();
            default:
                break;
        }

        // TODO. match record type
        return false;
    }
    else
    {
        // TODO. match generic type
        return false;
    }
}

// convert tools
bool V8BindTools::native_to_v8_primitive(
    ::v8::Local<::v8::Context> context,
    ::v8::Isolate*             isolate,
    rttr::TypeSignatureView    signature,
    void*                      native_data,
    ::v8::Local<::v8::Value>&  out_v8_value)
{
    if (signature.is_type() && !signature.is_decayed_pointer())
    {
        // get type id
        signature.jump_modifier();
        GUID type_id;
        signature.read_type_id(type_id);

        switch (type_id.get_hash())
        {
            case rttr::type_id_of<int8_t>().get_hash():
                out_v8_value = ::v8::Integer::New(isolate, *reinterpret_cast<int8_t*>(native_data));
                return true;
            case rttr::type_id_of<int16_t>().get_hash():
                out_v8_value = ::v8::Integer::New(isolate, *reinterpret_cast<int16_t*>(native_data));
                return true;
            case rttr::type_id_of<int32_t>().get_hash():
                out_v8_value = ::v8::Integer::New(isolate, *reinterpret_cast<int32_t*>(native_data));
                return true;
            case rttr::type_id_of<uint8_t>().get_hash():
                out_v8_value = ::v8::Integer::NewFromUnsigned(isolate, *reinterpret_cast<uint8_t*>(native_data));
                return true;
            case rttr::type_id_of<uint16_t>().get_hash():
                out_v8_value = ::v8::Integer::NewFromUnsigned(isolate, *reinterpret_cast<uint16_t*>(native_data));
                return true;
            case rttr::type_id_of<uint32_t>().get_hash():
                out_v8_value = ::v8::Integer::NewFromUnsigned(isolate, *reinterpret_cast<uint32_t*>(native_data));
                return true;
            case rttr::type_id_of<int64_t>().get_hash():
                out_v8_value = ::v8::BigInt::New(isolate, *reinterpret_cast<int64_t*>(native_data));
                return true;
            case rttr::type_id_of<uint64_t>().get_hash():
                out_v8_value = ::v8::BigInt::NewFromUnsigned(isolate, *reinterpret_cast<uint64_t*>(native_data));
                return true;
            case rttr::type_id_of<float>().get_hash():
                out_v8_value = ::v8::Number::New(isolate, *reinterpret_cast<float*>(native_data));
                return true;
            case rttr::type_id_of<double>().get_hash():
                out_v8_value = ::v8::Number::New(isolate, *reinterpret_cast<double*>(native_data));
                return true;
            case rttr::type_id_of<bool>().get_hash():
                out_v8_value = ::v8::Boolean::New(isolate, *reinterpret_cast<bool*>(native_data));
                return true;
            case rttr::type_id_of<skr::String>().get_hash():
                out_v8_value = ::v8::String::NewFromUtf8(isolate, reinterpret_cast<skr::String*>(native_data)->c_str_raw()).ToLocalChecked();
                return true;
            default:
                break;
        }
    }
    return false;
}
bool V8BindTools::v8_to_native_primitive(
    ::v8::Local<::v8::Context> context,
    ::v8::Isolate*             isolate,
    rttr::TypeSignatureView    signature,
    ::v8::Local<::v8::Value>   v8_value,
    void*                      out_native_data,
    bool                       is_init
)
{
    // only handle native type
    if (signature.is_type() && !signature.is_decayed_pointer())
    {
        // get type id
        signature.jump_modifier();
        GUID type_id;
        signature.read_type_id(type_id);

        // match primitive types
        switch (type_id.get_hash())
        {
            case rttr::type_id_of<int8_t>().get_hash():
                *reinterpret_cast<int8_t*>(out_native_data) = static_cast<int8_t>(v8_value->Int32Value(context).ToChecked());
                return true;
            case rttr::type_id_of<int16_t>().get_hash():
                *reinterpret_cast<int16_t*>(out_native_data) = static_cast<int16_t>(v8_value->Int32Value(context).ToChecked());
                return true;
            case rttr::type_id_of<int32_t>().get_hash():
                *reinterpret_cast<int32_t*>(out_native_data) = v8_value->Int32Value(context).ToChecked();
                return true;
            case rttr::type_id_of<uint8_t>().get_hash():
                *reinterpret_cast<uint8_t*>(out_native_data) = static_cast<uint8_t>(v8_value->Uint32Value(context).ToChecked());
                return true;
            case rttr::type_id_of<uint16_t>().get_hash():
                *reinterpret_cast<uint16_t*>(out_native_data) = static_cast<uint16_t>(v8_value->Uint32Value(context).ToChecked());
                return true;
            case rttr::type_id_of<uint32_t>().get_hash():
                *reinterpret_cast<uint32_t*>(out_native_data) = v8_value->Uint32Value(context).ToChecked();
                return true;
            case rttr::type_id_of<int64_t>().get_hash():
                *reinterpret_cast<int64_t*>(out_native_data) = v8_value->ToBigInt(context).ToLocalChecked()->Int64Value();
                return true;
            case rttr::type_id_of<uint64_t>().get_hash():
                *reinterpret_cast<uint64_t*>(out_native_data) = v8_value->ToBigInt(context).ToLocalChecked()->Uint64Value();
                return true;
            case rttr::type_id_of<float>().get_hash():
                *reinterpret_cast<float*>(out_native_data) = static_cast<float>(v8_value->NumberValue(context).ToChecked());
                return true;
            case rttr::type_id_of<double>().get_hash():
                *reinterpret_cast<double*>(out_native_data) = v8_value->NumberValue(context).ToChecked();
                return true;
            case rttr::type_id_of<bool>().get_hash():
                *reinterpret_cast<bool*>(out_native_data) = v8_value->BooleanValue(isolate);
                return true;
            case rttr::type_id_of<skr::String>().get_hash():
                if (is_init)
                {
                    *reinterpret_cast<skr::String*>(out_native_data) = skr::String::From(*::v8::String::Utf8Value(isolate, v8_value));
                }
                else
                {
                    new (out_native_data) skr::String(
                        skr::String::From(*::v8::String::Utf8Value(isolate, v8_value))
                    );
                }
                return true;
            default:
                break;
        }
    }
    return false;
}

// function invoke helpers
void V8BindTools::push_param(
    rttr::DynamicStack&        stack,
    const rttr::ParamData&     param_data,
    ::v8::Local<::v8::Context> context,
    ::v8::Isolate*             isolate,
    ::v8::Local<::v8::Value>   v8_value
)
{
    auto native_signature = param_data.type.view();

    if (v8_value.IsEmpty() || v8_value->IsUndefined())
    { // TODO. default value
        SKR_UNIMPLEMENTED_FUNCTION()
    }
    else if (native_signature.is_type())
    { // export record type
        if (native_signature.is_decayed_pointer())
        { // export ref/pointer
            // TODO. 区分指针和引用，指针可控，引用不可空
            // check ref level
            auto ref_level = native_signature.decayed_pointer_level();
            SKR_ASSERT(ref_level <= 1 && "only support ref/pointer level <= 1");

            // read type id
            native_signature.jump_modifier();
            GUID type_id;
            native_signature.read_type_id(type_id);

            // export primitive
            bool success_export_primitive = _push_param_primitive(
                stack,
                type_id,
                rttr::EDynamicStackParamKind::XValue,
                v8_value,
                context,
                isolate
            );

            // TODO. export record
            if (!success_export_primitive)
            {
                SKR_UNIMPLEMENTED_FUNCTION()
            }
        }
        else
        { // export param
            // read type id
            native_signature.jump_modifier();
            GUID type_id;
            native_signature.read_type_id(type_id);

            // export primitive
            bool success_export_primitive = _push_param_primitive(
                stack,
                type_id,
                rttr::EDynamicStackParamKind::Direct,
                v8_value,
                context,
                isolate
            );

            // TODO. export record
            if (!success_export_primitive)
            {
                SKR_UNIMPLEMENTED_FUNCTION()
            }
        }
    }
    else
    { // TODO. export generic type
        SKR_UNIMPLEMENTED_FUNCTION()
    }
}
bool V8BindTools::read_return(
    rttr::DynamicStack&        stack, 
    rttr::TypeSignatureView    signature,
    ::v8::Local<::v8::Context> context,
    ::v8::Isolate*             isolate,
    ::v8::Local<::v8::Value>&  out_value
)
{
    if (!stack.is_return_stored())
    {
        return false;
    }

    if (signature.is_type())
    {
        if (signature.is_decayed_pointer())
        {
            // TODO. support ref return
            SKR_UNIMPLEMENTED_FUNCTION()
        }
        else
        {
            if (native_to_v8_primitive(
                context,
                isolate,
                signature,
                stack.get_return_raw(),
                out_value
            ))
            {
                return true;
            }
            else
            {
                // TODO. handle record type
                SKR_UNIMPLEMENTED_FUNCTION()
            }
        }
    }
    else 
    {
        // TODO. handle generic type
        SKR_UNIMPLEMENTED_FUNCTION()
    }
    return false;
}

// caller
void V8BindTools::call_ctor(
    void*                                          obj,
    const rttr::CtorData&                          data,
    const ::v8::FunctionCallbackInfo<::v8::Value>& info,
    ::v8::Local<::v8::Context>                     context,
    ::v8::Isolate*                                 isolate
)
{
    rttr::DynamicStack stack;

    // combine stack
    for (size_t i = 0; i < data.param_data.size(); ++i)
    {
        push_param(
            stack,
            data.param_data[i],
            context,
            isolate,
            i < info.Length() ? info[i] : ::v8::Local<::v8::Value>{}
        );
    }

    // invoke
    stack.return_behaviour_discard();
    data.dynamic_stack_invoke(obj, stack);
}
void V8BindTools::call_method(
    void*                                          obj,
    const Vector<rttr::ParamData>&                 params,
    const rttr::TypeSignatureView                  ret_type,
    rttr::MethodInvokerDynamicStack                invoker,
    const ::v8::FunctionCallbackInfo<::v8::Value>& info,
    ::v8::Local<::v8::Context>                     context,
    ::v8::Isolate*                                 isolate
)
{
    rttr::DynamicStack stack;

    // combine proxy
    for (size_t i = 0; i < params.size(); ++i)
    {
        push_param(
            stack,
            params[i],
            context,
            isolate,
            i < info.Length() ? info[i] : ::v8::Local<::v8::Value>{}
        );
    }

    // invoke
    stack.return_behaviour_store();
    invoker(obj, stack);

    // read return
    ::v8::Local<::v8::Value> out_value;
    if (read_return(
        stack,
        ret_type,
        context,
        isolate,
        out_value
    ))
    {
        info.GetReturnValue().Set(out_value);
    }
}
void V8BindTools::call_function(
    const Vector<rttr::ParamData>&                 params,
    const rttr::TypeSignatureView                  ret_type,
    rttr::FuncInvokerDynamicStack                  invoker,
    const ::v8::FunctionCallbackInfo<::v8::Value>& info,
    ::v8::Local<::v8::Context>                     context,
    ::v8::Isolate*                                 isolate
)
{
    rttr::DynamicStack stack;

    // push param
    for (size_t i = 0; i < params.size(); ++i)
    {
        push_param(
            stack,
            params[i],
            context,
            isolate,
            i < info.Length() ? info[i] : ::v8::Local<::v8::Value>{}
        );
    }

    // invoke
    stack.return_behaviour_store();
    invoker(stack);

    // read return
    ::v8::Local<::v8::Value> out_value;
    if (read_return(
        stack,
        ret_type,
        context,
        isolate,
        out_value
    ))
    {
        info.GetReturnValue().Set(out_value);
    }
}
} // namespace skr::v8