#include "SkrV8/v8_bind.hpp"
#include "v8-external.h"

// util helpers
namespace skr::v8_bind
{
inline void* get_field_address(
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
} // namespace skr::v8_bind

// match helper
namespace skr::v8_bind
{
bool _match_primitive(const ScriptBinderPrimitive& binder, v8::Local<v8::Value> v8_value)
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
        return v8_value->IsBigInt();
    case type_id_of<uint8_t>().get_hash():
        return v8_value->IsUint32();
    case type_id_of<uint16_t>().get_hash():
        return v8_value->IsUint32();
    case type_id_of<uint32_t>().get_hash():
        return v8_value->IsUint32();
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
        SKR_UNREACHABLE_CODE()
        return false;
    }
}
bool _match_box(const ScriptBinderBox& binder, v8::Local<v8::Value> v8_value)
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
            v8_bind::to_v8(field_name, true)
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
bool _match_wrap(const ScriptBinderWrap& binder, v8::Local<v8::Value> v8_value)
{
    auto isolate = v8::Isolate::GetCurrent();
    auto context = isolate->GetCurrentContext();

    auto* type = binder.type;

    // check v8 value type
    if (!v8_value->IsObject()) { return false; }
    auto v8_object = v8_value->ToObject(context).ToLocalChecked();

    // check wrap
    if (v8_object->InternalFieldCount() < 1) { return false; }
    void*             raw_bind_core = v8_object->GetInternalField(0).As<v8::External>()->Value();
    V8BindRecordCore* bind_core     = reinterpret_cast<V8BindRecordCore*>(raw_bind_core);

    // check inherit
    return bind_core->type->based_on(type->type_id());
}
} // namespace skr::v8_bind

// convert helpers
namespace skr::v8_bind
{
v8::Local<v8::Value> _to_v8_primitive(
    const ScriptBinderPrimitive& binder,
    void*                        native_data
)
{
    switch (binder.type_id.get_hash())
    {
    case type_id_of<int8_t>().get_hash():
        return v8_bind::to_v8(*reinterpret_cast<int8_t*>(native_data));
    case type_id_of<int16_t>().get_hash():
        return v8_bind::to_v8(*reinterpret_cast<int16_t*>(native_data));
    case type_id_of<int32_t>().get_hash():
        return v8_bind::to_v8(*reinterpret_cast<int32_t*>(native_data));
    case type_id_of<int64_t>().get_hash():
        return v8_bind::to_v8(*reinterpret_cast<int64_t*>(native_data));
    case type_id_of<uint8_t>().get_hash():
        return v8_bind::to_v8(*reinterpret_cast<uint8_t*>(native_data));
    case type_id_of<uint16_t>().get_hash():
        return v8_bind::to_v8(*reinterpret_cast<uint16_t*>(native_data));
    case type_id_of<uint32_t>().get_hash():
        return v8_bind::to_v8(*reinterpret_cast<uint32_t*>(native_data));
    case type_id_of<uint64_t>().get_hash():
        return v8_bind::to_v8(*reinterpret_cast<uint64_t*>(native_data));
    case type_id_of<float>().get_hash():
        return v8_bind::to_v8(*reinterpret_cast<float*>(native_data));
    case type_id_of<double>().get_hash():
        return v8_bind::to_v8(*reinterpret_cast<double*>(native_data));
    case type_id_of<bool>().get_hash():
        return v8_bind::to_v8(*reinterpret_cast<bool*>(native_data));
    case type_id_of<skr::String>().get_hash():
        return v8_bind::to_v8(*reinterpret_cast<skr::String*>(native_data));
    default:
        SKR_UNREACHABLE_CODE()
        return {};
    }
}
bool _to_native_primitive(
    const ScriptBinderPrimitive& binder,
    v8::Local<v8::Value>         v8_value,
    void*                        native_data,
    bool                         is_init
)
{
    switch (binder.type_id.get_hash())
    {
    case type_id_of<int8_t>().get_hash():
        return v8_bind::to_native(v8_value, *reinterpret_cast<int8_t*>(native_data));
    case type_id_of<int16_t>().get_hash():
        return v8_bind::to_native(v8_value, *reinterpret_cast<int16_t*>(native_data));
    case type_id_of<int32_t>().get_hash():
        return v8_bind::to_native(v8_value, *reinterpret_cast<int32_t*>(native_data));
    case type_id_of<int64_t>().get_hash():
        return v8_bind::to_native(v8_value, *reinterpret_cast<int64_t*>(native_data));
    case type_id_of<uint8_t>().get_hash():
        return v8_bind::to_native(v8_value, *reinterpret_cast<uint8_t*>(native_data));
    case type_id_of<uint16_t>().get_hash():
        return v8_bind::to_native(v8_value, *reinterpret_cast<uint16_t*>(native_data));
    case type_id_of<uint32_t>().get_hash():
        return v8_bind::to_native(v8_value, *reinterpret_cast<uint32_t*>(native_data));
    case type_id_of<uint64_t>().get_hash():
        return v8_bind::to_native(v8_value, *reinterpret_cast<uint64_t*>(native_data));
    case type_id_of<float>().get_hash():
        return v8_bind::to_native(v8_value, *reinterpret_cast<float*>(native_data));
    case type_id_of<double>().get_hash():
        return v8_bind::to_native(v8_value, *reinterpret_cast<double*>(native_data));
    case type_id_of<bool>().get_hash():
        return v8_bind::to_native(v8_value, *reinterpret_cast<bool*>(native_data));
    case type_id_of<skr::String>().get_hash():
        if (!is_init)
        {
            new (native_data) skr::String();
        }
        return v8_bind::to_native(v8_value, *reinterpret_cast<skr::String*>(native_data));
    default:
        SKR_UNREACHABLE_CODE()
        return false;
    }
}
v8::Local<v8::Value> _to_v8_box(
    const ScriptBinderBox& binder,
    void*                  obj
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
            v8_bind::to_v8(field_name, true),
            v8_field_value
        ).Check();
        // clang-format on
    }
    return result;
}
bool _to_native_box(
    const ScriptBinderBox& binder,
    v8::Local<v8::Value>   v8_value,
    void*                  native_data,
    bool                   is_init
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
        // find object field
        // clang-format off
        auto v8_field = v8_object->Get(
            context,
            v8_bind::to_v8(field_name)
        ).ToLocalChecked();
        // clang-format on

        set_field(
            field_data,
            v8_field,
            native_data,
            type
        );
    }

    return false;
}
v8::Local<v8::Value> _to_v8_wrap(
    const ScriptBinderWrap& binder,
    void*                   native_data
)
{
    auto isolate     = v8::Isolate::GetCurrent();
    auto skr_isolate = reinterpret_cast<V8Isolate*>(isolate->GetData(0));

    auto* type             = binder.type;
    void* cast_raw         = type->cast_to_base(type_id_of<ScriptbleObject>(), native_data);
    auto* scriptble_object = reinterpret_cast<ScriptbleObject*>(cast_raw);

    auto* bind_core = skr_isolate->translate_record(scriptble_object);
    return bind_core->v8_object.Get(isolate);
}
bool _to_native_wrap(
    const ScriptBinderWrap& binder,
    v8::Local<v8::Value>    v8_value,
    void*                   native_data,
    bool                    is_init
)
{
    auto isolate = v8::Isolate::GetCurrent();
    auto context = isolate->GetCurrentContext();

    auto* type = binder.type;

    auto  v8_object     = v8_value->ToObject(context).ToLocalChecked();
    void* raw_bind_core = v8_object->GetInternalField(0).As<v8::External>()->Value();
    auto* bind_core     = reinterpret_cast<V8BindRecordCore*>(raw_bind_core);

    // do cast
    void* cast_ptr                         = bind_core->cast_to_base(type->type_id());
    *reinterpret_cast<void**>(native_data) = cast_ptr;
    return true;
}
} // namespace skr::v8_bind

// invoke helper
namespace skr::v8_bind
{
void _push_param(
    DynamicStack&            stack,
    const ScriptBinderParam& param_binder,
    v8::Local<v8::Value>     v8_value
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
        to_native(param_binder.binder, native_data, v8_value, false);
        break;
    }
    case ScriptBinderRoot::EKind::Box: {
        auto*       box         = param_binder.binder.box();
        auto        dtor_data   = box->type->dtor_data();
        DtorInvoker dtor        = dtor_data.has_value() ? dtor_data.value().native_invoke : nullptr;
        void*       native_data = stack.alloc_param_raw(
            box->type->size(),
            box->type->alignment(),
            param_binder.pass_by_ref ? EDynamicStackParamKind::XValue : EDynamicStackParamKind::Direct,
            dtor
        );
        to_native(param_binder.binder, native_data, v8_value, false);
        break;
    }
    case ScriptBinderRoot::EKind::Wrap: {
        void* native_data = stack.alloc_param_raw(
            sizeof(void*),
            alignof(void*),
            EDynamicStackParamKind::Direct,
            nullptr
        );
        to_native(param_binder.binder, native_data, v8_value, false);
        break;
    }
    default:
        SKR_UNREACHABLE_CODE()
        break;
    }
}
v8::Local<v8::Value> read_return(
    DynamicStack&             stack,
    const ScriptBinderReturn& return_binder
)
{
    if (!stack.is_return_stored()) { return {}; }
    void* native_data;
    switch (return_binder.binder.kind())
    {
    case ScriptBinderRoot::EKind::Primitive:
    case ScriptBinderRoot::EKind::Box: {
        void* raw_data = stack.get_return_raw();
        native_data    = *reinterpret_cast<void**>(raw_data);
        break;
    }
    case ScriptBinderRoot::EKind::Wrap: {
        native_data = stack.get_return_raw();
        break;
    }
    default:
        SKR_UNREACHABLE_CODE()
        return {};
    }
    return to_v8(return_binder.binder, native_data);
}
} // namespace skr::v8_bind

// top-level functional
namespace skr::v8_bind
{
// field tools
bool set_field(
    const ScriptBinderField& binder,
    v8::Local<v8::Value>     v8_value,
    void*                    obj,
    const RTTRType*          obj_type
)
{
    // get field info
    void* field_address = get_field_address(binder.data, binder.owner, obj_type, obj);

    // to native
    return to_native(binder.binder, field_address, v8_value, true);
}
bool set_field(
    const ScriptBinderStaticField& binder,
    v8::Local<v8::Value>           v8_value
)
{
    // get field info
    void* field_address = binder.data->address;

    // to native
    return to_native(binder.binder, field_address, v8_value, true);
}
v8::Local<v8::Value> get_field(
    const ScriptBinderField& binder,
    const void*              obj,
    const RTTRType*          obj_type
)
{
    // get field info
    void* field_address = get_field_address(
        binder.data,
        binder.owner,
        obj_type,
        const_cast<void*>(obj)
    );

    // to v8
    return to_v8(binder.binder, field_address);
}
v8::Local<v8::Value> get_field(
    const ScriptBinderStaticField& binder
)
{
    // get field info
    void* field_address = binder.data->address;

    // to v8
    return to_v8(binder.binder, field_address);
}

// call native
bool call_native(
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
bool call_native(
    const ScriptBinderMethod&                      binder,
    const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack,
    void*                                          obj,
    const RTTRType*                                obj_type
)
{
    for (const auto& overload : binder.overloads)
    {
        if (!match(overload.params_binder, v8_stack)) { continue; }

        DynamicStack native_stack;

        // push param
        for (uint32_t i = 0; i < v8_stack.Length(); ++i)
        {
            _push_param(
                native_stack,
                overload.params_binder[i],
                v8_stack[i]
            );
        }

        // cast
        void* owner_address = obj_type->cast_to_base(overload.owner->type_id(), obj);

        // invoke
        native_stack.return_behaviour_store();
        overload.data->dynamic_stack_invoke(owner_address, native_stack);

        // read return
        if (native_stack.is_return_stored())
        {
            v8::Local<v8::Value> out_value = read_return(
                native_stack,
                overload.return_binder
            );
            v8_stack.GetReturnValue().Set(out_value);
        }

        return true;
    }

    return false;
}
bool call_native(
    const ScriptBinderStaticMethod&                binder,
    const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack
)
{
    for (const auto& overload : binder.overloads)
    {
        if (!match(overload.params_binder, v8_stack)) { continue; }

        DynamicStack native_stack;

        // push param
        for (uint32_t i = 0; i < v8_stack.Length(); ++i)
        {
            _push_param(
                native_stack,
                overload.params_binder[i],
                v8_stack[i]
            );
        }

        // invoke
        native_stack.return_behaviour_store();
        overload.data->dynamic_stack_invoke(native_stack);

        // read return
        if (native_stack.is_return_stored())
        {
            v8::Local<v8::Value> out_value = read_return(
                native_stack,
                overload.return_binder
            );
            v8_stack.GetReturnValue().Set(out_value);
        }
    }

    return false;
}
} // namespace skr::v8_bind

// raw convert and match
namespace skr::v8_bind
{
// convert
v8::Local<v8::Value> to_v8(
    ScriptBinderRoot binder,
    void*            native_data
)
{
    switch (binder.kind())
    {
    case ScriptBinderRoot::EKind::Primitive:
        return _to_v8_primitive(*binder.primitive(), native_data);
    case ScriptBinderRoot::EKind::Box:
        return _to_v8_box(*binder.box(), native_data);
    case ScriptBinderRoot::EKind::Wrap:
        return _to_v8_wrap(*binder.wrap(), native_data);
    }
    return {};
}
bool to_native(
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
    case ScriptBinderRoot::EKind::Box:
        return _to_native_box(*binder.box(), v8_value, native_data, is_init);
    case ScriptBinderRoot::EKind::Wrap:
        return _to_native_wrap(*binder.wrap(), v8_value, native_data, is_init);
    }
    return {};
}

// match type
bool match(
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
    case ScriptBinderRoot::EKind::Box:
        return _match_primitive(*binder.primitive(), v8_value);
    case ScriptBinderRoot::EKind::Wrap:
        return _match_wrap(*binder.wrap(), v8_value);
    }

    return false;
}
bool match(
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
bool match(
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
bool match(
    const ScriptBinderField& binder,
    v8::Local<v8::Value>     v8_value
)
{
    // check null
    if (v8_value.IsEmpty() || v8_value->IsNull() || v8_value->IsUndefined())
    {
        return binder.binder.is_wrap();
    }

    // check type
    return match(binder.binder, v8_value);
}
bool match(
    const ScriptBinderStaticField& binder,
    v8::Local<v8::Value>           v8_value
)
{
    // check null
    if (v8_value.IsEmpty() || v8_value->IsNull() || v8_value->IsUndefined())
    {
        return binder.binder.is_wrap();
    }

    // check type
    return match(binder.binder, v8_value);
}
bool match(
    const Vector<ScriptBinderParam>&               param_binders,
    const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack
)
{
    auto isolate = v8::Isolate::GetCurrent();
    auto context = isolate->GetCurrentContext();

    // check param count
    if (param_binders.size() != v8_stack.Length()) { return false; }

    // check param type
    for (size_t i = 0; i < param_binders.size(); i++)
    {
        auto& binder = param_binders[i];
        auto  v8_arg = v8_stack[i];

        if (!match(binder, v8_arg)) { return false; }
    }

    return true;
}
} // namespace skr::v8_bind