#include "SkrV8/v8_bind.hpp"
#include "v8-external.h"
#include "v8-container.h"

// helpres
namespace skr
{
// match helper
V8MethodMatchResult V8Bind::_match_primitive(const ScriptBinderPrimitive& binder, v8::Local<v8::Value> v8_value)
{
    switch (binder.type_id.get_hash())
    {
    case type_id_of<int8_t>().get_hash():
        return { v8_value->IsInt32(), 0 };
    case type_id_of<int16_t>().get_hash():
        return { v8_value->IsInt32(), 0 };
    case type_id_of<int32_t>().get_hash():
        return { v8_value->IsInt32(), 0 };
    case type_id_of<int64_t>().get_hash():
        if (v8_value->IsBigInt())
        {
            return { true, 0 };
        }
        else if (v8_value->IsInt32() || v8_value->IsUint32())
        {
            return { true, -1 };
        }
        else
        {
            return {};
        }
    case type_id_of<uint8_t>().get_hash():
        return { v8_value->IsUint32(), 0 };
    case type_id_of<uint16_t>().get_hash():
        return { v8_value->IsUint32(), 0 };
    case type_id_of<uint32_t>().get_hash():
        return { v8_value->IsUint32(), 0 };
    case type_id_of<uint64_t>().get_hash():
        if (v8_value->IsBigInt())
        {
            return { true, 0 };
        }
        else if (v8_value->IsInt32() || v8_value->IsUint32())
        {
            return { true, -1 };
        }
        else
        {
            return {};
        }
    case type_id_of<float>().get_hash():
        return { v8_value->IsNumber(), 0 };
    case type_id_of<double>().get_hash():
        return { v8_value->IsNumber(), 0 };
    case type_id_of<bool>().get_hash():
        return { v8_value->IsBoolean(), 0 };
    case type_id_of<skr::String>().get_hash():
        return { v8_value->IsString(), 0 };
    case type_id_of<skr::StringView>().get_hash():
        return { v8_value->IsString(), 0 };
    default:
        SKR_UNREACHABLE_CODE()
        return {};
    }
}
V8MethodMatchResult V8Bind::_match_mapping(const ScriptBinderMapping& binder, v8::Local<v8::Value> v8_value)
{
    auto isolate = v8::Isolate::GetCurrent();
    auto context = isolate->GetCurrentContext();

    // conv to object
    if (!v8_value->IsObject()) { return {}; }
    auto v8_obj = v8_value->ToObject(context).ToLocalChecked();

    // match fields
    int32_t score = 0;
    for (const auto& [field_name, field_data] : binder.fields)
    {
        // find object field
        auto v8_field_value_maybe = v8_obj->Get(
            context,
            to_v8(field_name, true)
        );
        // clang-format on

        // get field value
        if (v8_field_value_maybe.IsEmpty()) { return {}; }
        auto v8_field_value = v8_field_value_maybe.ToLocalChecked();

        // match field
        auto result = match(field_data.binder, v8_field_value);
        if (!result.matched) { return {}; }

        score += result.match_score + 1;
    }

    return { true, score };
}
V8MethodMatchResult V8Bind::_match_object(const ScriptBinderObject& binder, v8::Local<v8::Value> v8_value)
{
    auto isolate = v8::Isolate::GetCurrent();
    auto context = isolate->GetCurrentContext();

    // check v8 value type
    if (!v8_value->IsObject()) { return {}; }
    auto v8_object = v8_value->ToObject(context).ToLocalChecked();

    // check object
    if (v8_object->InternalFieldCount() < 1) { return {}; }
    void* raw_bind_core = v8_object->GetInternalField(0).As<v8::External>()->Value();
    auto* bind_core     = reinterpret_cast<V8BindCoreRecordBase*>(raw_bind_core);
    if (!bind_core->is_valid()) { return {}; }

    // check inherit
    uint32_t cast_count = 0;
    bool     base_on    = bind_core->type->based_on(binder.type->type_id(), &cast_count);
    return { base_on, (int32_t)cast_count };
}
V8MethodMatchResult V8Bind::_match_value(const ScriptBinderValue& binder, v8::Local<v8::Value> v8_value)
{
    auto isolate = v8::Isolate::GetCurrent();
    auto context = isolate->GetCurrentContext();

    // check v8 value type
    if (!v8_value->IsObject()) { return {}; }
    auto v8_object = v8_value->ToObject(context).ToLocalChecked();

    // check object
    if (v8_object->InternalFieldCount() < 1) { return {}; }
    void* raw_bind_core = v8_object->GetInternalField(0).As<v8::External>()->Value();
    auto* bind_core     = reinterpret_cast<V8BindCoreRecordBase*>(raw_bind_core);
    if (!bind_core->is_valid()) { return {}; }

    // check inherit
    uint32_t cast_count = 0;
    bool     base_on    = bind_core->type->based_on(binder.type->type_id());
    return { base_on, (int32_t)cast_count };
}
} // namespace skr

namespace skr
{
// match type
V8MethodMatchResult V8Bind::match(
    ScriptBinderRoot     binder,
    v8::Local<v8::Value> v8_value
)
{
    if (binder.is_empty()) { return {}; }

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
    case ScriptBinderRoot::EKind::Value:
        return _match_value(*binder.value(), v8_value);
    default:
        return {};
    }
}
V8MethodMatchResult V8Bind::match(
    const ScriptBinderParam& binder,
    v8::Local<v8::Value>     v8_value
)
{
    // check null
    if (v8_value.IsEmpty() || v8_value->IsNull() || v8_value->IsUndefined())
    {
        // only object is nullable
        return { binder.binder.is_object(), 0 };
    }

    return match(binder.binder, v8_value);
}
V8MethodMatchResult V8Bind::match(
    const ScriptBinderReturn& binder,
    v8::Local<v8::Value>      v8_value
)
{
    // check null
    if (v8_value.IsEmpty() || v8_value->IsNull() || v8_value->IsUndefined())
    {
        // only object is nullable
        return { binder.binder.is_object(), 0 };
    }

    return match(binder.binder, v8_value);
}
V8MethodMatchResult V8Bind::match(
    const ScriptBinderField& binder,
    v8::Local<v8::Value>     v8_value
)
{
    // check null
    if (v8_value.IsEmpty() || v8_value->IsNull() || v8_value->IsUndefined())
    {
        return { binder.binder.is_object(), 0 };
    }

    // check type
    return match(binder.binder, v8_value);
}
V8MethodMatchResult V8Bind::match(
    const ScriptBinderStaticField& binder,
    v8::Local<v8::Value>           v8_value
)
{
    // check null
    if (v8_value.IsEmpty() || v8_value->IsNull() || v8_value->IsUndefined())
    {
        return { binder.binder.is_object(), 0 };
    }

    // check type
    return match(binder.binder, v8_value);
}
V8MethodMatchResult V8Bind::match(
    const Vector<ScriptBinderParam>&               param_binders,
    uint32_t                                       solved_param_count,
    const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack
)
{
    // check param count
    if (solved_param_count != v8_stack.Length()) { return {}; }

    // check param type
    int32_t score = 0;
    for (size_t i = 0; i < param_binders.size(); i++)
    {
        auto& binder = param_binders[i];
        auto  v8_arg = v8_stack[i];

        // skip pure out param
        if (binder.inout_flag == ERTTRParamFlag::Out) { continue; }

        auto result = match(binder, v8_arg);
        if (!result.matched) { return {}; }
        score += result.match_score;
    }

    return { true, score };
}
} // namespace skr