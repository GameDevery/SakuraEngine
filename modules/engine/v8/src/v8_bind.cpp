#include "SkrV8/v8_bind.hpp"
#include "v8-external.h"
#include "v8-container.h"

// helpres
namespace skr
{
// util helpers
void* V8Bind::_get_field_address(
    const RTTRFieldData* field,
    const RTTRType*      field_owner,
    const RTTRType*      obj_type,
    void*                obj
)
{
    SKR_ASSERT(obj != nullptr);
    void* field_owner_address = obj_type->cast_to_base(field_owner->type_id(), obj);
    return field->get_address(field_owner_address);
}

// match helper
bool V8Bind::_match_primitive(const ScriptBinderPrimitive& binder, v8::Local<v8::Value> v8_value)
{
    switch (binder.type_id.get_hash())
    {
    case type_id_of<int8_t>().get_hash():
        return v8_value->IsInt32();
    case type_id_of<int16_t>().get_hash():
        return v8_value->IsInt32();
    case type_id_of<int32_t>().get_hash():
        return v8_value->IsInt32();
    case type_id_of<int64_t>().get_hash():
        return v8_value->IsBigInt() || v8_value->IsInt32() || v8_value->IsUint32();
    case type_id_of<uint8_t>().get_hash():
        return v8_value->IsUint32();
    case type_id_of<uint16_t>().get_hash():
        return v8_value->IsUint32();
    case type_id_of<uint32_t>().get_hash():
        return v8_value->IsUint32();
    case type_id_of<uint64_t>().get_hash():
        return v8_value->IsBigInt() || v8_value->IsUint32();
    case type_id_of<float>().get_hash():
        return v8_value->IsNumber();
    case type_id_of<double>().get_hash():
        return v8_value->IsNumber();
    case type_id_of<bool>().get_hash():
        return v8_value->IsBoolean();
    case type_id_of<skr::String>().get_hash():
        return v8_value->IsString();
    case type_id_of<skr::StringView>().get_hash():
        return v8_value->IsString();
    default:
        SKR_UNREACHABLE_CODE()
        return false;
    }
}
bool V8Bind::_match_mapping(const ScriptBinderMapping& binder, v8::Local<v8::Value> v8_value)
{
    auto isolate = v8::Isolate::GetCurrent();
    auto context = isolate->GetCurrentContext();

    // conv to object
    if (!v8_value->IsObject()) { return false; }
    auto v8_obj = v8_value->ToObject(context).ToLocalChecked();

    // match fields
    for (const auto& [field_name, field_data] : binder.fields)
    {
        // find object field
        auto v8_field_value_maybe = v8_obj->Get(
            context,
            to_v8(field_name, true)
        );
        // clang-format on

        // get field value
        if (v8_field_value_maybe.IsEmpty()) { return false; }
        auto v8_field_value = v8_field_value_maybe.ToLocalChecked();

        // match field
        if (!match(field_data.binder, v8_field_value)) { return false; }
    }

    return true;
}
bool V8Bind::_match_object(const ScriptBinderObject& binder, v8::Local<v8::Value> v8_value)
{
    auto isolate = v8::Isolate::GetCurrent();
    auto context = isolate->GetCurrentContext();

    auto* type = binder.type;

    // check v8 value type
    if (!v8_value->IsObject()) { return false; }
    auto v8_object = v8_value->ToObject(context).ToLocalChecked();

    // check object
    if (v8_object->InternalFieldCount() < 1) { return false; }
    void* raw_bind_core = v8_object->GetInternalField(0).As<v8::External>()->Value();
    auto* bind_core     = reinterpret_cast<V8BindCoreRecordBase*>(raw_bind_core);

    // check inherit
    return bind_core->type->based_on(type->type_id());
}

// convert helper
v8::Local<v8::Value> V8Bind::_to_v8_primitive(
    const ScriptBinderPrimitive& binder,
    void*                        native_data
)
{
    switch (binder.type_id.get_hash())
    {
    case type_id_of<int8_t>().get_hash():
        return to_v8(*reinterpret_cast<int8_t*>(native_data));
    case type_id_of<int16_t>().get_hash():
        return to_v8(*reinterpret_cast<int16_t*>(native_data));
    case type_id_of<int32_t>().get_hash():
        return to_v8(*reinterpret_cast<int32_t*>(native_data));
    case type_id_of<int64_t>().get_hash():
        return to_v8(*reinterpret_cast<int64_t*>(native_data));
    case type_id_of<uint8_t>().get_hash():
        return to_v8(*reinterpret_cast<uint8_t*>(native_data));
    case type_id_of<uint16_t>().get_hash():
        return to_v8(*reinterpret_cast<uint16_t*>(native_data));
    case type_id_of<uint32_t>().get_hash():
        return to_v8(*reinterpret_cast<uint32_t*>(native_data));
    case type_id_of<uint64_t>().get_hash():
        return to_v8(*reinterpret_cast<uint64_t*>(native_data));
    case type_id_of<float>().get_hash():
        return to_v8(*reinterpret_cast<float*>(native_data));
    case type_id_of<double>().get_hash():
        return to_v8(*reinterpret_cast<double*>(native_data));
    case type_id_of<bool>().get_hash():
        return to_v8(*reinterpret_cast<bool*>(native_data));
    case type_id_of<skr::String>().get_hash():
        return to_v8(*reinterpret_cast<skr::String*>(native_data));
    case type_id_of<skr::StringView>().get_hash():
        return to_v8(*reinterpret_cast<skr::StringView*>(native_data));
    default:
        SKR_UNREACHABLE_CODE()
        return {};
    }
}
void V8Bind::_init_primitive(
    const ScriptBinderPrimitive& binder,
    void*                        native_data
)
{
    switch (binder.type_id.get_hash())
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
bool V8Bind::_to_native_primitive(
    const ScriptBinderPrimitive& binder,
    v8::Local<v8::Value>         v8_value,
    void*                        native_data,
    bool                         is_init
)
{
    if (!is_init)
    {
        _init_primitive(binder, native_data);
    }

    switch (binder.type_id.get_hash())
    {
    case type_id_of<int8_t>().get_hash():
        return to_native(v8_value, *reinterpret_cast<int8_t*>(native_data));
    case type_id_of<int16_t>().get_hash():
        return to_native(v8_value, *reinterpret_cast<int16_t*>(native_data));
    case type_id_of<int32_t>().get_hash():
        return to_native(v8_value, *reinterpret_cast<int32_t*>(native_data));
    case type_id_of<int64_t>().get_hash():
        return to_native(v8_value, *reinterpret_cast<int64_t*>(native_data));
    case type_id_of<uint8_t>().get_hash():
        return to_native(v8_value, *reinterpret_cast<uint8_t*>(native_data));
    case type_id_of<uint16_t>().get_hash():
        return to_native(v8_value, *reinterpret_cast<uint16_t*>(native_data));
    case type_id_of<uint32_t>().get_hash():
        return to_native(v8_value, *reinterpret_cast<uint32_t*>(native_data));
    case type_id_of<uint64_t>().get_hash():
        return to_native(v8_value, *reinterpret_cast<uint64_t*>(native_data));
    case type_id_of<float>().get_hash():
        return to_native(v8_value, *reinterpret_cast<float*>(native_data));
    case type_id_of<double>().get_hash():
        return to_native(v8_value, *reinterpret_cast<double*>(native_data));
    case type_id_of<bool>().get_hash():
        return to_native(v8_value, *reinterpret_cast<bool*>(native_data));
    case type_id_of<skr::String>().get_hash():
        return to_native(v8_value, *reinterpret_cast<skr::String*>(native_data));
    default:
        SKR_UNREACHABLE_CODE()
        return false;
    }
}
v8::Local<v8::Value> V8Bind::_to_v8_mapping(
    const ScriptBinderMapping& binder,
    void*                      obj
)
{
    auto isolate = v8::Isolate::GetCurrent();
    auto context = isolate->GetCurrentContext();

    auto* type = binder.type;

    auto result = v8::Object::New(isolate);
    for (const auto& [field_name, field_data] : binder.fields)
    {
        // to v8
        auto v8_field_value = get_field(field_data, obj, type);

        // set object field
        // clang-format off
        result->Set(
            context,
            to_v8(field_name, true),
            v8_field_value
        ).Check();
        // clang-format on
    }
    return result;
}
bool V8Bind::_to_native_mapping(
    const ScriptBinderMapping& binder,
    v8::Local<v8::Value>       v8_value,
    void*                      native_data,
    bool                       is_init
)
{
    auto isolate = v8::Isolate::GetCurrent();
    auto context = isolate->GetCurrentContext();

    auto* type      = binder.type;
    auto  v8_object = v8_value->ToObject(context).ToLocalChecked();

    // do init
    if (!is_init)
    {
        void* raw_invoker = type->find_ctor_t<void()>()->native_invoke;
        auto* invoker     = reinterpret_cast<void (*)(void*)>(raw_invoker);
        invoker(native_data);
    }

    for (const auto& [field_name, field_data] : binder.fields)
    {
        // clang-format off
        // find object field
        auto v8_field = v8_object->Get(
            context,
            to_v8(field_name)
        ).ToLocalChecked();
        
        // set field
        if (!set_field(
            field_data,
            v8_field,
            native_data,
            type
        ))
        {
            return false;
        }
        // clang-format on
    }

    return true;
}
v8::Local<v8::Value> V8Bind::_to_v8_object(
    const ScriptBinderObject& binder,
    void*                     native_data
)
{
    auto isolate     = v8::Isolate::GetCurrent();
    auto skr_isolate = reinterpret_cast<V8Isolate*>(isolate->GetData(0));

    auto* type             = binder.type;
    void* cast_raw         = type->cast_to_base(type_id_of<ScriptbleObject>(), native_data);
    auto* scriptble_object = reinterpret_cast<ScriptbleObject*>(cast_raw);

    auto* bind_core = skr_isolate->translate_object(scriptble_object);
    return bind_core->v8_object.Get(isolate);
}
bool V8Bind::_to_native_object(
    const ScriptBinderObject& binder,
    v8::Local<v8::Value>      v8_value,
    void*                     native_data,
    bool                      is_init
)
{
    auto isolate = v8::Isolate::GetCurrent();
    auto context = isolate->GetCurrentContext();

    auto* type = binder.type;

    auto  v8_object     = v8_value->ToObject(context).ToLocalChecked();
    void* raw_bind_core = v8_object->GetInternalField(0).As<v8::External>()->Value();
    auto* bind_core     = reinterpret_cast<V8BindCoreObject*>(raw_bind_core);

    // do cast
    void* cast_ptr                         = bind_core->cast_to_base(type->type_id());
    *reinterpret_cast<void**>(native_data) = cast_ptr;
    return true;
}

// invoker helper
void V8Bind::_push_param(
    DynamicStack&            stack,
    const ScriptBinderParam& param_binder,
    v8::Local<v8::Value>     v8_value
)
{
    switch (param_binder.binder.kind())
    {
    case ScriptBinderRoot::EKind::Primitive: {
        auto* primitive = param_binder.binder.primitive();
        if (primitive->type_id == type_id_of<StringView>())
        {
            StringViewStackProxy* proxy = stack.alloc_param<StringViewStackProxy>(
                EDynamicStackParamKind::Direct,
                nullptr,
                StringViewStackProxy::custom_mapping
            );
            new (proxy) StringViewStackProxy();
            SKR_ASSERT(V8Bind::to_native(v8_value, proxy->holder));
            proxy->view = proxy->holder;
        }
        else
        {
            void* native_data = stack.alloc_param_raw(
                primitive->size,
                primitive->alignment,
                param_binder.pass_by_ref ? EDynamicStackParamKind::XValue : EDynamicStackParamKind::Direct,
                primitive->dtor
            );
            SKR_ASSERT(to_native(param_binder.binder, native_data, v8_value, false));
        }
        break;
    }
    case ScriptBinderRoot::EKind::Mapping: {
        auto*       mapping     = param_binder.binder.mapping();
        auto        dtor_data   = mapping->type->dtor_data();
        DtorInvoker dtor        = dtor_data.has_value() ? dtor_data.value().native_invoke : nullptr;
        void*       native_data = stack.alloc_param_raw(
            mapping->type->size(),
            mapping->type->alignment(),
            param_binder.pass_by_ref ? EDynamicStackParamKind::XValue : EDynamicStackParamKind::Direct,
            dtor
        );
        SKR_ASSERT(to_native(param_binder.binder, native_data, v8_value, false));
        break;
    }
    case ScriptBinderRoot::EKind::Object: {
        void* native_data = stack.alloc_param_raw(
            sizeof(void*),
            alignof(void*),
            EDynamicStackParamKind::Direct,
            nullptr
        );
        SKR_ASSERT(to_native(param_binder.binder, native_data, v8_value, false));
        break;
    }
    default:
        SKR_UNREACHABLE_CODE()
        break;
    }
}
void V8Bind::_push_param_pure_out(
    DynamicStack&            stack,
    const ScriptBinderParam& param_binder
)
{
    switch (param_binder.binder.kind())
    {
    case ScriptBinderRoot::EKind::Primitive: {
        auto* primitive   = param_binder.binder.primitive();
        void* native_data = stack.alloc_param_raw(
            primitive->size,
            primitive->alignment,
            param_binder.pass_by_ref ? EDynamicStackParamKind::XValue : EDynamicStackParamKind::Direct,
            primitive->dtor
        );

        // call ctor
        _init_primitive(*param_binder.binder.primitive(), native_data);
        break;
    }
    case ScriptBinderRoot::EKind::Mapping: {
        auto*       mapping     = param_binder.binder.mapping();
        auto        dtor_data   = mapping->type->dtor_data();
        DtorInvoker dtor        = dtor_data.has_value() ? dtor_data.value().native_invoke : nullptr;
        void*       native_data = stack.alloc_param_raw(
            mapping->type->size(),
            mapping->type->alignment(),
            param_binder.pass_by_ref ? EDynamicStackParamKind::XValue : EDynamicStackParamKind::Direct,
            dtor
        );

        // call ctor
        auto* ctor_data = mapping->type->find_ctor_t<void()>();
        auto* invoker   = reinterpret_cast<void (*)(void*)>(ctor_data->native_invoke);
        invoker(native_data);
        break;
    }
    case ScriptBinderRoot::EKind::Object: {
        SKR_UNREACHABLE_CODE()
        break;
    }
    default:
        SKR_UNREACHABLE_CODE()
        break;
    }
}
v8::Local<v8::Value> V8Bind::_read_return(
    DynamicStack&                    stack,
    const Vector<ScriptBinderParam>& params_binder,
    const ScriptBinderReturn&        return_binder,
    uint32_t                         solved_return_count
)
{
    auto* isolate = v8::Isolate::GetCurrent();
    auto  context = isolate->GetCurrentContext();

    if (solved_return_count == 1)
    { // return single value
        if (return_binder.is_void)
        { // read from out param
            for (const auto& param_binder : params_binder)
            {
                if (flag_all(param_binder.inout_flag, ERTTRParamFlag::Out))
                {
                    return _read_return_from_out_param(
                        stack,
                        param_binder
                    );
                }
            }
        }
        else
        { // read return value
            if (stack.is_return_stored())
            {
                return _read_return(
                    stack,
                    return_binder
                );
            }
        }
    }
    else
    { // return param array
        v8::Local<v8::Array> out_array = v8::Array::New(isolate);
        uint32_t             cur_index = 0;

        // try read return value
        if (!return_binder.is_void)
        {
            if (stack.is_return_stored())
            {
                v8::Local<v8::Value> out_value = _read_return(
                    stack,
                    return_binder
                );
                out_array->Set(context, cur_index, out_value).Check();
                ++cur_index;
            }
        }

        // read return value from out param
        for (const auto& param_binder : params_binder)
        {
            if (flag_all(param_binder.inout_flag, ERTTRParamFlag::Out))
            {
                // clang-format off
                    out_array->Set(context, cur_index,_read_return_from_out_param(
                        stack,
                        param_binder
                    )).Check();
                // clang-format on
                ++cur_index;
            }
        }

        return out_array;
    }

    return {};
}
v8::Local<v8::Value> V8Bind::_read_return(
    DynamicStack&             stack,
    const ScriptBinderReturn& return_binder
)
{
    if (!stack.is_return_stored()) { return {}; }

    void* native_data;
    switch (return_binder.binder.kind())
    {
    case ScriptBinderRoot::EKind::Primitive:
    case ScriptBinderRoot::EKind::Mapping: {
        native_data = stack.get_return_raw();
        if (return_binder.is_nullable)
        {
            native_data = *reinterpret_cast<void**>(native_data);
        }
        break;
    }
    case ScriptBinderRoot::EKind::Object: {
        native_data = stack.get_return_raw();
        break;
    }
    default:
        SKR_UNREACHABLE_CODE()
        return {};
    }
    return to_v8(return_binder.binder, native_data);
}
v8::Local<v8::Value> V8Bind::_read_return_from_out_param(
    DynamicStack&            stack,
    const ScriptBinderParam& param_binder
)
{
    void* native_data = stack.get_param_raw(param_binder.data->index);
    return to_v8(
        param_binder.binder,
        native_data
    );
}
} // namespace skr

namespace skr
{
// field tools
bool V8Bind::set_field(
    const ScriptBinderField& binder,
    v8::Local<v8::Value>     v8_value,
    void*                    obj,
    const RTTRType*          obj_type
)
{
    // get field info
    void* field_address = _get_field_address(binder.data, binder.owner, obj_type, obj);

    // to native
    return to_native(binder.binder, field_address, v8_value, true);
}
bool V8Bind::set_field(
    const ScriptBinderStaticField& binder,
    v8::Local<v8::Value>           v8_value
)
{
    // get field info
    void* field_address = binder.data->address;

    // to native
    return to_native(binder.binder, field_address, v8_value, true);
}
v8::Local<v8::Value> V8Bind::get_field(
    const ScriptBinderField& binder,
    const void*              obj,
    const RTTRType*          obj_type
)
{
    // get field info
    void* field_address = _get_field_address(
        binder.data,
        binder.owner,
        obj_type,
        const_cast<void*>(obj)
    );

    // to v8
    return to_v8(binder.binder, field_address);
}
v8::Local<v8::Value> V8Bind::get_field(
    const ScriptBinderStaticField& binder
)
{
    // get field info
    void* field_address = binder.data->address;

    // to v8
    return to_v8(binder.binder, field_address);
}

// call native
bool V8Bind::call_native(
    const ScriptBinderCtor&                        binder,
    const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack,
    void*                                          obj
)
{
    DynamicStack native_stack;

    // push param
    for (uint32_t i = 0; i < v8_stack.Length(); ++i)
    {
        _push_param(
            native_stack,
            binder.params_binder[i],
            v8_stack[i]
        );
    }

    // invoke
    native_stack.return_behaviour_discard();
    binder.data->dynamic_stack_invoke(obj, native_stack);

    return true;
}
bool V8Bind::call_native(
    const ScriptBinderMethod&                      binder,
    const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack,
    void*                                          obj,
    const RTTRType*                                obj_type
)
{
    auto* isolate = v8::Isolate::GetCurrent();
    auto  context = isolate->GetCurrentContext();

    for (const auto& overload : binder.overloads)
    {
        if (!match(overload.params_binder, overload.params_count, v8_stack)) { continue; }

        DynamicStack native_stack;

        // push param
        uint32_t v8_stack_index = 0;
        for (const auto& param_binder : overload.params_binder)
        {
            if (param_binder.inout_flag == ERTTRParamFlag::Out)
            { // pure out param, we will push a dummy xvalue
                _push_param_pure_out(
                    native_stack,
                    param_binder
                );
            }
            else
            {
                _push_param(
                    native_stack,
                    param_binder,
                    v8_stack[v8_stack_index]
                );
                ++v8_stack_index;
            }
        }

        // cast
        void* owner_address = obj_type->cast_to_base(overload.owner->type_id(), obj);

        // invoke
        native_stack.return_behaviour_store();
        overload.data->dynamic_stack_invoke(owner_address, native_stack);

        // read return
        auto return_value = _read_return(
            native_stack,
            overload.params_binder,
            overload.return_binder,
            overload.return_count
        );
        if (!return_value.IsEmpty())
        {
            v8_stack.GetReturnValue().Set(return_value);
        }

        return true;
    }

    return false;
}
bool V8Bind::call_native(
    const ScriptBinderStaticMethod&                binder,
    const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack
)
{
    for (const auto& overload : binder.overloads)
    {
        if (!match(overload.params_binder, overload.params_count, v8_stack)) { continue; }

        DynamicStack native_stack;

        // push param
        uint32_t v8_stack_index = 0;
        for (const auto& param_binder : overload.params_binder)
        {
            if (param_binder.inout_flag == ERTTRParamFlag::Out)
            { // pure out param, we will push a dummy xvalue
                _push_param_pure_out(
                    native_stack,
                    param_binder
                );
            }
            else
            {
                _push_param(
                    native_stack,
                    param_binder,
                    v8_stack[v8_stack_index]
                );
                ++v8_stack_index;
            }
        }

        // invoke
        native_stack.return_behaviour_store();
        overload.data->dynamic_stack_invoke(native_stack);

        // read return
        auto return_value = _read_return(
            native_stack,
            overload.params_binder,
            overload.return_binder,
            overload.return_count
        );
        if (!return_value.IsEmpty())
        {
            v8_stack.GetReturnValue().Set(return_value);
        }

        return true;
    }

    return false;
}

// convert
v8::Local<v8::Value> V8Bind::to_v8(
    ScriptBinderRoot binder,
    void*            native_data
)
{
    switch (binder.kind())
    {
    case ScriptBinderRoot::EKind::Primitive:
        return _to_v8_primitive(*binder.primitive(), native_data);
    case ScriptBinderRoot::EKind::Enum:
        return _to_v8_primitive(*binder.enum_()->underlying_binder, native_data);
    case ScriptBinderRoot::EKind::Mapping:
        return _to_v8_mapping(*binder.mapping(), native_data);
    case ScriptBinderRoot::EKind::Object:
        return _to_v8_object(*binder.object(), native_data);
    }
    return {};
}
bool V8Bind::to_native(
    ScriptBinderRoot     binder,
    void*                native_data,
    v8::Local<v8::Value> v8_value,
    bool                 is_init
)
{
    switch (binder.kind())
    {
    case ScriptBinderRoot::EKind::Primitive:
        return _to_native_primitive(*binder.primitive(), v8_value, native_data, is_init);
    case ScriptBinderRoot::EKind::Enum:
        return _to_native_primitive(*binder.enum_()->underlying_binder, v8_value, native_data, is_init);
    case ScriptBinderRoot::EKind::Mapping:
        return _to_native_mapping(*binder.mapping(), v8_value, native_data, is_init);
    case ScriptBinderRoot::EKind::Object:
        return _to_native_object(*binder.object(), v8_value, native_data, is_init);
    }
    return false;
}

// match type
bool V8Bind::match(
    ScriptBinderRoot     binder,
    v8::Local<v8::Value> v8_value
)
{
    if (binder.is_empty()) { return false; }

    auto isolate = v8::Isolate::GetCurrent();
    auto context = isolate->GetCurrentContext();

    switch (binder.kind())
    {
    case ScriptBinderRoot::EKind::Primitive:
        return _match_primitive(*binder.primitive(), v8_value);
    case ScriptBinderRoot::EKind::Enum:
        return _match_primitive(*binder.enum_()->underlying_binder, v8_value);
    case ScriptBinderRoot::EKind::Mapping:
        return _match_mapping(*binder.mapping(), v8_value);
    case ScriptBinderRoot::EKind::Object:
        return _match_object(*binder.object(), v8_value);
    }

    return false;
}
bool V8Bind::match(
    const ScriptBinderParam& binder,
    v8::Local<v8::Value>     v8_value
)
{
    // check null
    if (v8_value.IsEmpty() || v8_value->IsNull() || v8_value->IsUndefined())
    {
        return binder.is_nullable;
    }

    return match(binder.binder, v8_value);
}
bool V8Bind::match(
    const ScriptBinderReturn& binder,
    v8::Local<v8::Value>      v8_value
)
{
    // check null
    if (v8_value.IsEmpty() || v8_value->IsNull() || v8_value->IsUndefined())
    {
        return binder.is_nullable;
    }

    return match(binder.binder, v8_value);
}
bool V8Bind::match(
    const ScriptBinderField& binder,
    v8::Local<v8::Value>     v8_value
)
{
    // check null
    if (v8_value.IsEmpty() || v8_value->IsNull() || v8_value->IsUndefined())
    {
        return binder.binder.is_object();
    }

    // check type
    return match(binder.binder, v8_value);
}
bool V8Bind::match(
    const ScriptBinderStaticField& binder,
    v8::Local<v8::Value>           v8_value
)
{
    // check null
    if (v8_value.IsEmpty() || v8_value->IsNull() || v8_value->IsUndefined())
    {
        return binder.binder.is_object();
    }

    // check type
    return match(binder.binder, v8_value);
}
bool V8Bind::match(
    const Vector<ScriptBinderParam>&               param_binders,
    uint32_t                                       solved_param_count,
    const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack
)
{
    auto isolate = v8::Isolate::GetCurrent();
    auto context = isolate->GetCurrentContext();

    // check param count
    if (solved_param_count != v8_stack.Length()) { return false; }

    // check param type
    for (size_t i = 0; i < param_binders.size(); i++)
    {
        auto& binder = param_binders[i];
        auto  v8_arg = v8_stack[i];

        // skip pure out param
        if (binder.inout_flag == ERTTRParamFlag::Out) { continue; }

        if (!match(binder, v8_arg)) { return false; }
    }

    return true;
}
} // namespace skr