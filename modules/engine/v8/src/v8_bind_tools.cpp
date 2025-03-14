#include "SkrV8/v8_bind_tools.hpp"
#include "v8-isolate.h"

// helpers
namespace skr
{
// write primitive
inline static bool _push_param_primitive(
    DynamicStack&              stack,
    skr::GUID                  type_id,
    EDynamicStackParamKind     kind,
    ::v8::Local<::v8::Value>   v8_value,
    ::v8::Local<::v8::Context> context,
    ::v8::Isolate*             isolate
)
{
    switch (type_id.get_hash())
    {
    // int32/uint32
    case type_id_of<int8_t>().get_hash():
        stack.add_param<int8_t>((int8_t)v8_value->Int32Value(context).ToChecked(), kind);
        return true;
    case type_id_of<int16_t>().get_hash():
        stack.add_param<int16_t>((int16_t)v8_value->Int32Value(context).ToChecked(), kind);
        return true;
    case type_id_of<int32_t>().get_hash():
        stack.add_param<int32_t>((int32_t)v8_value->Int32Value(context).ToChecked(), kind);
        return true;
    case type_id_of<uint8_t>().get_hash():
        stack.add_param<uint8_t>((uint8_t)v8_value->Uint32Value(context).ToChecked(), kind);
        return true;
    case type_id_of<uint16_t>().get_hash():
        stack.add_param<uint16_t>((uint16_t)v8_value->Uint32Value(context).ToChecked(), kind);
        return true;
    case type_id_of<uint32_t>().get_hash():
        stack.add_param<uint32_t>((uint32_t)v8_value->Uint32Value(context).ToChecked(), kind);
        return true;
    // int64/uint64
    case type_id_of<int64_t>().get_hash():
        stack.add_param<int64_t>((int64_t)v8_value->ToBigInt(context).ToLocalChecked()->Int64Value(), kind);
        return true;
    case type_id_of<uint64_t>().get_hash():
        stack.add_param<uint64_t>((uint64_t)v8_value->ToBigInt(context).ToLocalChecked()->Uint64Value(), kind);
        return true;
    // float/double
    case type_id_of<float>().get_hash():
        stack.add_param<float>((float)v8_value->NumberValue(context).ToChecked(), kind);
        return true;
    case type_id_of<double>().get_hash():
        stack.add_param<double>((double)v8_value->NumberValue(context).ToChecked(), kind);
        return true;
    // bool
    case type_id_of<bool>().get_hash():
        stack.add_param<bool>(v8_value->BooleanValue(isolate), kind);
        return true;
    // string
    case type_id_of<skr::String>().get_hash():
        stack.add_param<skr::String>(skr::String::From(*::v8::String::Utf8Value(isolate, v8_value)), kind);
        return true;
    default:
        return false;
    }
}
template <typename Data>
inline static bool _match_params(const Data* data, const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;
    auto call_length   = info.Length();
    auto native_length = data->param_data.size();

    // length > data, must be unmatched
    if (call_length > native_length)
    {
        return false;
    }

    // match default param
    for (size_t i = call_length; i < native_length; ++i)
    {
        if (!data->param_data[i]->make_default)
        {
            return false;
        }
    }

    // match signature
    for (size_t i = 0; i < call_length; ++i)
    {
        Local<Value>      call_value       = info[i];
        TypeSignatureView native_signature = data->param_data[i]->type.view();

        // use default value
        if (call_value->IsUndefined())
        {
            if (!data->param_data[i]->make_default)
            {
                return false;
            }
        }

        // match type
        if (!V8BindTools::match_type(call_value, native_signature))
        {
            return false;
        }
    }

    return true;
}
} // namespace skr

namespace skr
{
// match tools
bool V8BindTools::match_type(::v8::Local<::v8::Value> v8_value, TypeSignatureView native_signature)
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
        case type_id_of<int8_t>().get_hash():
            return v8_value->IsInt32();
        case type_id_of<int16_t>().get_hash():
            return v8_value->IsInt32();
        case type_id_of<int32_t>().get_hash():
            return v8_value->IsInt32();
        case type_id_of<uint8_t>().get_hash():
            return v8_value->IsUint32();
        case type_id_of<uint16_t>().get_hash():
            return v8_value->IsUint32();
        case type_id_of<uint32_t>().get_hash():
            return v8_value->IsUint32();
        case type_id_of<int64_t>().get_hash():
            return v8_value->IsBigInt();
        case type_id_of<uint64_t>().get_hash():
            return v8_value->IsBigInt();
        case type_id_of<float>().get_hash():
            return v8_value->IsNumber();
        case type_id_of<double>().get_hash():
            return v8_value->IsNumber();
        case type_id_of<bool>().get_hash():
            return v8_value->IsBoolean();
        case type_id_of<skr::String>().get_hash():
            return v8_value->IsString();
        default:
            break;
        }

        // start match use type id
        RTTRType* type = get_type_from_guid(type_id);
        if (!type) { return false; }

        // isolate and context
        auto isolate = ::v8::Isolate::GetCurrent();
        auto context = isolate->GetCurrentContext();

        // match box type
        if (flag_all(type->record_flag(), ERTTRRecordFlag::ScriptBox))
        {
            if (v8_value->IsObject() || v8_value->IsArray())
            {
                auto v8_obj = v8_value->ToObject(context).ToLocalChecked();

                // match fields
                bool failed = false;
                type->each_field([&](const RTTRFieldData* field, const RTTRType* field_owner) {
                    // filter flag
                    if (!flag_all(field->flag, ERTTRFieldFlag::ScriptVisible)) { return; }

                    // fast exit if failed
                    if (failed) { return; }

                    // match field
                    auto v8_obj_field = v8_obj->Get(context, str_to_v8(field->name, isolate));
                    if (v8_obj_field.IsEmpty())
                    {
                        failed = true;
                        return;
                    }
                    if (!match_type(v8_obj_field.ToLocalChecked(), field->type))
                    {
                        failed = true;
                        return;
                    }
                });

                // check if success
                if (!failed)
                {
                    return true;
                }
            }
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
bool V8BindTools::match_params(const RTTRCtorData* data, const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    return _match_params(data, info);
}
bool V8BindTools::match_params(const RTTRMethodData* data, const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    return _match_params(data, info);
}
bool V8BindTools::match_params(const RTTRStaticMethodData* data, const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    return _match_params(data, info);
}
bool V8BindTools::match_params(const RTTRExternMethodData* data, const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    return _match_params(data, info);
}
bool V8BindTools::match_params(const RTTRFunctionData* data, const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    return _match_params(data, info);
}

// convert tools
bool V8BindTools::native_to_v8_primitive(
    ::v8::Local<::v8::Context> context,
    ::v8::Isolate*             isolate,
    TypeSignatureView          signature,
    void*                      native_data,
    ::v8::Local<::v8::Value>&  out_v8_value
)
{
    if (signature.is_type() && !signature.is_decayed_pointer())
    {
        // get type id
        signature.jump_modifier();
        GUID type_id;
        signature.read_type_id(type_id);

        switch (type_id.get_hash())
        {
        case type_id_of<int8_t>().get_hash():
            out_v8_value = ::v8::Integer::New(isolate, *reinterpret_cast<int8_t*>(native_data));
            return true;
        case type_id_of<int16_t>().get_hash():
            out_v8_value = ::v8::Integer::New(isolate, *reinterpret_cast<int16_t*>(native_data));
            return true;
        case type_id_of<int32_t>().get_hash():
            out_v8_value = ::v8::Integer::New(isolate, *reinterpret_cast<int32_t*>(native_data));
            return true;
        case type_id_of<uint8_t>().get_hash():
            out_v8_value = ::v8::Integer::NewFromUnsigned(isolate, *reinterpret_cast<uint8_t*>(native_data));
            return true;
        case type_id_of<uint16_t>().get_hash():
            out_v8_value = ::v8::Integer::NewFromUnsigned(isolate, *reinterpret_cast<uint16_t*>(native_data));
            return true;
        case type_id_of<uint32_t>().get_hash():
            out_v8_value = ::v8::Integer::NewFromUnsigned(isolate, *reinterpret_cast<uint32_t*>(native_data));
            return true;
        case type_id_of<int64_t>().get_hash():
            out_v8_value = ::v8::BigInt::New(isolate, *reinterpret_cast<int64_t*>(native_data));
            return true;
        case type_id_of<uint64_t>().get_hash():
            out_v8_value = ::v8::BigInt::NewFromUnsigned(isolate, *reinterpret_cast<uint64_t*>(native_data));
            return true;
        case type_id_of<float>().get_hash():
            out_v8_value = ::v8::Number::New(isolate, *reinterpret_cast<float*>(native_data));
            return true;
        case type_id_of<double>().get_hash():
            out_v8_value = ::v8::Number::New(isolate, *reinterpret_cast<double*>(native_data));
            return true;
        case type_id_of<bool>().get_hash():
            out_v8_value = ::v8::Boolean::New(isolate, *reinterpret_cast<bool*>(native_data));
            return true;
        case type_id_of<skr::String>().get_hash():
            out_v8_value = V8BindTools::str_to_v8(*reinterpret_cast<skr::String*>(native_data), isolate, false);
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
    TypeSignatureView          signature,
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
        case type_id_of<int8_t>().get_hash():
            *reinterpret_cast<int8_t*>(out_native_data) = static_cast<int8_t>(v8_value->Int32Value(context).ToChecked());
            return true;
        case type_id_of<int16_t>().get_hash():
            *reinterpret_cast<int16_t*>(out_native_data) = static_cast<int16_t>(v8_value->Int32Value(context).ToChecked());
            return true;
        case type_id_of<int32_t>().get_hash():
            *reinterpret_cast<int32_t*>(out_native_data) = v8_value->Int32Value(context).ToChecked();
            return true;
        case type_id_of<uint8_t>().get_hash():
            *reinterpret_cast<uint8_t*>(out_native_data) = static_cast<uint8_t>(v8_value->Uint32Value(context).ToChecked());
            return true;
        case type_id_of<uint16_t>().get_hash():
            *reinterpret_cast<uint16_t*>(out_native_data) = static_cast<uint16_t>(v8_value->Uint32Value(context).ToChecked());
            return true;
        case type_id_of<uint32_t>().get_hash():
            *reinterpret_cast<uint32_t*>(out_native_data) = v8_value->Uint32Value(context).ToChecked();
            return true;
        case type_id_of<int64_t>().get_hash():
            *reinterpret_cast<int64_t*>(out_native_data) = v8_value->ToBigInt(context).ToLocalChecked()->Int64Value();
            return true;
        case type_id_of<uint64_t>().get_hash():
            *reinterpret_cast<uint64_t*>(out_native_data) = v8_value->ToBigInt(context).ToLocalChecked()->Uint64Value();
            return true;
        case type_id_of<float>().get_hash():
            *reinterpret_cast<float*>(out_native_data) = static_cast<float>(v8_value->NumberValue(context).ToChecked());
            return true;
        case type_id_of<double>().get_hash():
            *reinterpret_cast<double*>(out_native_data) = v8_value->NumberValue(context).ToChecked();
            return true;
        case type_id_of<bool>().get_hash():
            *reinterpret_cast<bool*>(out_native_data) = v8_value->BooleanValue(isolate);
            return true;
        case type_id_of<skr::String>().get_hash():
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
bool V8BindTools::native_to_v8_box(
    ::v8::Local<::v8::Context> context,
    ::v8::Isolate*             isolate,
    TypeSignatureView          signature,
    void*                      native_data,
    ::v8::Local<::v8::Value>&  out_v8_value
)
{
    // check signature
    if (!signature.is_type() || signature.is_decayed_pointer()) { return false; }

    // get type
    signature.jump_modifier();
    GUID type_id;
    signature.read_type_id(type_id);
    RTTRType* type = get_type_from_guid(type_id);
    if (!type) { return false; }

    // check box flag
    if (!::skr::flag_all(type->record_flag(), ERTTRRecordFlag::ScriptBox)) { return false; }

    auto result = ::v8::Object::New(isolate);

    // each fields
    type->each_field([&](const RTTRFieldData* field, const RTTRType* field_owner) {
        // check visible
        if (!flag_all(field->flag, ERTTRFieldFlag::ScriptVisible)) { return; }

        // get field info
        void* field_owner_address = type->cast_to_base(field_owner->type_id(), native_data);
        void* field_address       = field->get_address(field_owner_address);

        // convert type
        ::v8::Local<::v8::Value> field_value;
        if (native_to_v8_primitive(
                context,
                isolate,
                field->type,
                field_address,
                field_value
            ))
        {
            // set value
            result->Set(
                      context,
                      str_to_v8(field->name, isolate, true),
                      field_value
            )
                .Check();
        }
        else if (native_to_v8_box(
                     context,
                     isolate,
                     field->type,
                     field_address,
                     field_value
                 ))
        {
            // set value
            result->Set(
                      context,
                      str_to_v8(field->name, isolate, true),
                      field_value
            )
                .Check();
        }
        else
        {
            SKR_ASSERT(false && "ScriptBox record fields must be primitive or box type");
        }
    });

    // set value
    out_v8_value = result;

    return true;
}
bool V8BindTools::v8_to_native_box(
    ::v8::Local<::v8::Context> context,
    ::v8::Isolate*             isolate,
    TypeSignatureView          signature,
    ::v8::Local<::v8::Value>   v8_value,
    void*                      out_native_data,
    bool                       is_init
)
{
    // check signature
    if (!signature.is_type() || signature.is_decayed_pointer()) { return false; }

    // get type
    signature.jump_modifier();
    GUID type_id;
    signature.read_type_id(type_id);
    RTTRType* type = get_type_from_guid(type_id);
    if (!type) { return false; }

    // check box flag
    if (!::skr::flag_all(type->record_flag(), ERTTRRecordFlag::ScriptBox)) { return false; }

    // check v8 value
    if (!v8_value->IsObject() && !v8_value->IsArray())
    {
        return false;
    }
    auto v8_object = v8_value->ToObject(context).ToLocalChecked();

    // do init
    if (!is_init)
    {
        void* raw_invoker = type->find_ctor_t<void()>()->native_invoke;
        auto* invoker     = reinterpret_cast<void (*)(void*)>(raw_invoker);
        invoker(out_native_data);
    }

    // each fields
    bool failed = false;
    type->each_field([&](const RTTRFieldData* field, const RTTRType* field_owner) {
        // check visible
        if (!flag_all(field->flag, ERTTRFieldFlag::ScriptVisible)) { return; }

        // fast exit if failed
        if (failed) { return; }

        // get field info
        void* field_owner_address = type->cast_to_base(field_owner->type_id(), out_native_data);
        void* field_address       = field->get_address(field_owner_address);

        // find object field
        auto field_value = v8_object->Get(
            context,
            str_to_v8(field->name, isolate)
        );
        if (field_value.IsEmpty())
        {
            failed = true;
            return;
        }
        auto checked_field_value = field_value.ToLocalChecked();

        // do convert
        if (v8_to_native_primitive(
                context,
                isolate,
                field->type,
                checked_field_value,
                field_address,
                true
            ))
        {
        }
        else if (v8_to_native_box(
                     context,
                     isolate,
                     field->type,
                     checked_field_value,
                     field_address,
                     true
                 ))
        {
        }
        else
        {
            SKR_ASSERT(false && "ScriptBox record fields must be primitive or box type");
        }
    });

    return true;
}

// function invoke helpers
void V8BindTools::push_param(
    DynamicStack&              stack,
    const RTTRParamData&       param_data,
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
            if (_push_param_primitive(
                    stack,
                    type_id,
                    EDynamicStackParamKind::XValue,
                    v8_value,
                    context,
                    isolate
                ))
            {
                return;
            }

            // prepare use reflect export
            RTTRType* type = get_type_from_guid(type_id);
            if (!type) { return; }

            // try box export
            if (match_type(v8_value, native_signature))
            {
                // push stack
                auto  stack_data  = type->dtor_data();
                auto  dtor        = stack_data.has_value() ? stack_data.value().native_invoke : nullptr;
                void* native_data = stack.alloc_param_raw(
                    type->size(),
                    type->alignment(),
                    EDynamicStackParamKind::XValue,
                    dtor
                );

                // v8 to native
                if (v8_to_native_box(
                        context,
                        isolate,
                        native_signature,
                        v8_value,
                        native_data,
                        true
                    ))
                {
                    return;
                }
            }

            // TODO. export record
            SKR_UNIMPLEMENTED_FUNCTION()
        }
        else
        { // export param
            // read type id
            native_signature.jump_modifier();
            GUID type_id;
            native_signature.read_type_id(type_id);

            // export primitive
            if (_push_param_primitive(
                    stack,
                    type_id,
                    EDynamicStackParamKind::Direct,
                    v8_value,
                    context,
                    isolate
                ))
            {
                return;
            }

            // prepare use reflect export
            RTTRType* type = get_type_from_guid(type_id);
            if (!type) { return; }

            // try box export
            if (match_type(v8_value, native_signature))
            {
                // push stack
                auto  stack_data  = type->dtor_data();
                auto  dtor        = stack_data.has_value() ? stack_data.value().native_invoke : nullptr;
                void* native_data = stack.alloc_param_raw(
                    type->size(),
                    type->alignment(),
                    EDynamicStackParamKind::Direct,
                    dtor
                );

                // v8 to native
                if (v8_to_native_box(
                        context,
                        isolate,
                        native_signature,
                        v8_value,
                        native_data,
                        true
                    ))
                {
                    return;
                }
            }

            // TODO. export record
            SKR_UNIMPLEMENTED_FUNCTION()
        }
    }
    else
    { // TODO. export generic type
        SKR_UNIMPLEMENTED_FUNCTION()
    }
}
bool V8BindTools::read_return(
    DynamicStack&              stack,
    TypeSignatureView          signature,
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
            else if (native_to_v8_box(
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
    const RTTRCtorData&                            data,
    const ::v8::FunctionCallbackInfo<::v8::Value>& info,
    ::v8::Local<::v8::Context>                     context,
    ::v8::Isolate*                                 isolate
)
{
    DynamicStack stack;

    // combine stack
    for (size_t i = 0; i < data.param_data.size(); ++i)
    {
        push_param(
            stack,
            *data.param_data[i],
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
    const Vector<RTTRParamData*>&                  params,
    const TypeSignatureView                        ret_type,
    MethodInvokerDynamicStack                      invoker,
    const ::v8::FunctionCallbackInfo<::v8::Value>& info,
    ::v8::Local<::v8::Context>                     context,
    ::v8::Isolate*                                 isolate
)
{
    DynamicStack stack;

    // combine proxy
    for (size_t i = 0; i < params.size(); ++i)
    {
        push_param(
            stack,
            *params[i],
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
    const Vector<RTTRParamData*>&                  params,
    const TypeSignatureView                        ret_type,
    FuncInvokerDynamicStack                        invoker,
    const ::v8::FunctionCallbackInfo<::v8::Value>& info,
    ::v8::Local<::v8::Context>                     context,
    ::v8::Isolate*                                 isolate
)
{
    DynamicStack stack;

    // push param
    for (size_t i = 0; i < params.size(); ++i)
    {
        push_param(
            stack,
            *params[i],
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
} // namespace skr