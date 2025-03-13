#include "SkrV8/v8_matcher.hpp"
#include "SkrCore/log.hpp"
#include "SkrV8/v8_bind_tools.hpp"
#include "v8-external.h"

namespace skr
{
// match
V8MatchSuggestion V8Matcher::match_to_native(::v8::Local<::v8::Value> v8_value, rttr::TypeSignatureView signature)
{
    auto isolate = ::v8::Isolate::GetCurrent();
    auto context = isolate->GetCurrentContext();

    if (signature.is_type())
    {
        // read info
        bool is_decayed_pointer = signature.is_decayed_pointer();
        signature.jump_modifier();
        GUID type_id;
        signature.read_type_id(type_id);

        // match primitive
        V8MatchSuggestion result = _suggest_primitive(type_id);
        if (!result.is_empty())
        {
            if (_match_primitive(result, v8_value))
            {
                return result;
            }
        }

        // match box
        rttr::Type* type = rttr::get_type_from_guid(type_id);
        result           = _suggest_box(type);
        if (!result.is_empty())
        {
            if (_match_box(result, v8_value))
            {
                return result;
            }
        }

        // match wrap
        if (is_decayed_pointer)
        { // wrap export requires decayed pointer type
            result = _suggest_wrap(type);
            if (!result.is_empty())
            {
                if (_match_wrap(result, v8_value))
                {
                    return result;
                }
            }
        }
    }
    else if (signature.is_generic_type())
    {
        // TODO. support generic
    }

    return {};
}
V8MatchSuggestion V8Matcher::match_to_v8(rttr::TypeSignatureView signature)
{
    if (signature.is_type())
    {
        // read info
        bool is_decayed_pointer = signature.is_decayed_pointer();
        signature.jump_modifier();
        GUID type_id;
        signature.read_type_id(type_id);

        // match primitive
        V8MatchSuggestion result = _suggest_primitive(type_id);
        if (!result.is_empty()) return result;

        // match box
        rttr::Type* type = rttr::get_type_from_guid(type_id);
        result           = _suggest_box(type);
        if (!result.is_empty()) { return result; }

        // match wrap
        if (is_decayed_pointer)
        { // wrap export requires decayed pointer type
            result = _suggest_wrap(type);
            if (!result.is_empty())
            {
                return result;
            }
        }
    }
    else if (signature.is_generic_type())
    {
        // TODO. support generic
    }

    return {};
}

// convert
::v8::Local<::v8::Value> V8Matcher::conv_to_v8(
    const V8MatchSuggestion& suggestion,
    void*                    native_data
)
{
    switch (suggestion.kind())
    {
    case skr::V8MatchSuggestion::EKind::Primitive:
        return _to_v8_primitive(suggestion, native_data);
    case skr::V8MatchSuggestion::EKind::Box:
        return _to_v8_box(suggestion, native_data);
    case skr::V8MatchSuggestion::EKind::Wrap:
        return _to_v8_wrap(suggestion, native_data);
    default:
        SKR_UNREACHABLE_CODE()
        return {};
    }
}
bool V8Matcher::conv_to_native(
    const V8MatchSuggestion& suggestion,
    void*                    native_data,
    ::v8::Local<::v8::Value> v8_value,
    bool                     is_init
)
{
    switch (suggestion.kind())
    {
    case skr::V8MatchSuggestion::EKind::Primitive:
        return _to_native_primitive(suggestion, v8_value, native_data, is_init);
    case skr::V8MatchSuggestion::EKind::Box:
        return _to_native_box(suggestion, v8_value, native_data, is_init);
    case skr::V8MatchSuggestion::EKind::Wrap:
        return _to_native_wrap(suggestion, v8_value, native_data, is_init);
    default:
        SKR_UNREACHABLE_CODE()
        return false;
    }
}

// invoke convert
bool V8Matcher::call_native_push_params(
    const Vector<V8MatchSuggestion>&           suggestions,
    const Vector<rttr::ParamData*>&            params,
    rttr::DynamicStack&                        stack,
    const v8::FunctionCallbackInfo<v8::Value>& v8_func_info
)
{
    // TODO. check above

    auto isolate = v8::Isolate::GetCurrent();
    auto context = isolate->GetCurrentContext();

    for (size_t i = 0; i < suggestions.size(); ++i)
    {
        const auto& suggestion       = suggestions[i];
        auto        native_signature = params[i]->type.view();
        auto        v8_value         = v8_func_info[i];

        auto is_pointer         = native_signature.is_pointer();
        auto is_decayed_pointer = native_signature.is_decayed_pointer();

        switch (suggestion.kind())
        {
        case skr::V8MatchSuggestion::EKind::Primitive: {
            auto primitive_data = suggestion.primitive();

            // check null
            if (v8_value.IsEmpty() || v8_value->IsNullOrUndefined())
            {
                if (is_pointer)
                { // nullable
                    stack.add_param<void*>(nullptr, rttr::EDynamicStackParamKind::Direct);
                    return true;
                }
                else
                { // not null
                    isolate->ThrowError("value cannot be null/undefined");
                    return false;
                }
            }
            else
            {
                // do convert
                void* native_data = stack.alloc_param_raw(
                    primitive_data.alignment,
                    primitive_data.size,
                    is_decayed_pointer ? rttr::EDynamicStackParamKind::XValue : rttr::EDynamicStackParamKind::Direct,
                    primitive_data.dtor
                );
                if (!conv_to_native(suggestion, native_data, v8_value, false)) return false;
            }
        }
        case skr::V8MatchSuggestion::EKind::Box: {
            auto box_data = suggestion.box();

            // check null
            if (v8_value.IsEmpty() || v8_value->IsNullOrUndefined())
            {
                if (is_pointer)
                { // nullable
                    stack.add_param<void*>(nullptr, rttr::EDynamicStackParamKind::Direct);
                    return true;
                }
                else
                { // not null
                    isolate->ThrowError("value cannot be null/undefined");
                    return false;
                }
            }
            else
            {
                // do convert
                void* native_data = stack.alloc_param_raw(
                    box_data.type->alignment(),
                    box_data.type->size(),
                    is_decayed_pointer ? rttr::EDynamicStackParamKind::XValue : rttr::EDynamicStackParamKind::Direct,
                    nullptr
                );
                if (!conv_to_native(suggestion, native_data, v8_value, false)) return false;
            }
        }
        case skr::V8MatchSuggestion::EKind::Wrap: {
            auto wrap_data = suggestion.wrap();

            // check type
            if (!is_decayed_pointer)
            { // value type is not allowed
                isolate->ThrowError("wrap type must be pointer or reference");
                return false;
            }

            // check null
            if (v8_value.IsEmpty() || v8_value->IsNullOrUndefined())
            {
                if (is_pointer)
                { // nullable
                    stack.add_param<void*>(nullptr, rttr::EDynamicStackParamKind::Direct);
                    return true;
                }
                else
                { // not null
                    isolate->ThrowError("value cannot be null/undefined");
                    return false;
                }
            }
            else
            {
                // do convert
                void* native_data = stack.alloc_param_raw(
                    sizeof(void*),
                    alignof(void*),
                    rttr::EDynamicStackParamKind::Direct,
                    nullptr
                );
                if (!conv_to_native(suggestion, native_data, v8_value, false)) return false;
            }
        }
        }
    }

    return false;
}
v8::Local<v8::Value> V8Matcher::call_native_read_return(
    V8MatchSuggestion&  suggestion,
    rttr::DynamicStack& stack
)
{
    // TODO. check above
    
    if (!stack.is_return_stored()) { return {}; }

    conv_to_v8(suggestion, stack.get_return_raw());

    return {};
}

// match helper
V8MatchSuggestion V8Matcher::_suggest_primitive(GUID type_id)
{
    switch (type_id.get_hash())
    {
    case rttr::type_id_of<int8_t>().get_hash():
        return V8MatchSuggestion::FromPrimitive<int8_t>();
    case rttr::type_id_of<int16_t>().get_hash():
        return V8MatchSuggestion::FromPrimitive<int16_t>();
    case rttr::type_id_of<int32_t>().get_hash():
        return V8MatchSuggestion::FromPrimitive<int32_t>();
    case rttr::type_id_of<int64_t>().get_hash():
        return V8MatchSuggestion::FromPrimitive<int64_t>();
    case rttr::type_id_of<uint8_t>().get_hash():
        return V8MatchSuggestion::FromPrimitive<uint8_t>();
    case rttr::type_id_of<uint16_t>().get_hash():
        return V8MatchSuggestion::FromPrimitive<uint16_t>();
    case rttr::type_id_of<uint32_t>().get_hash():
        return V8MatchSuggestion::FromPrimitive<uint32_t>();
    case rttr::type_id_of<uint64_t>().get_hash():
        return V8MatchSuggestion::FromPrimitive<uint64_t>();
    case rttr::type_id_of<float>().get_hash():
        return V8MatchSuggestion::FromPrimitive<float>();
    case rttr::type_id_of<double>().get_hash():
        return V8MatchSuggestion::FromPrimitive<double>();
    case rttr::type_id_of<bool>().get_hash():
        return V8MatchSuggestion::FromPrimitive<bool>();
    case rttr::type_id_of<skr::String>().get_hash():
        return V8MatchSuggestion::FromPrimitive<skr::String>();
    default:
        return {};
    }
}
V8MatchSuggestion V8Matcher::_suggest_box(rttr::Type* type)
{
    if (!flag_all(type->record_flag(), rttr::ERecordFlag::ScriptBox)) { return {}; }

    V8MatchSuggestion result     = V8MatchSuggestion::FromBox(type);
    auto&             box_result = result.box();

    // each field
    type->each_field([&](const rttr::FieldData* field, const rttr::Type* owner_type) {
        // check visible
        if (!flag_all(field->flag, rttr::EFieldFlag::ScriptVisible)) { return; }

        // get type signature
        auto signature = field->type.view();

        // try export
        if (signature.is_decayed_pointer())
        {
            // TODO. support wrap type
        }
        else
        {
            skr::GUID type_id;
            signature.jump_modifier();
            signature.read_type_id(type_id);

            // try primitive
            auto suggestion = _suggest_primitive(type_id);
            if (!suggestion.is_empty())
            {
                box_result.primitive_members.push_back({
                    .field       = field,
                    .field_owner = owner_type,
                    .suggestion  = suggestion,
                });
                return;
            }

            // try box
            auto* field_type  = rttr::get_type_from_guid(type_id);
            auto  suggestions = _suggest_box(field_type);
            if (!suggestions.is_empty())
            {
                box_result.box_members.push_back({
                    .field       = field,
                    .field_owner = owner_type,
                    .suggestion  = suggestions,
                });
                return;
            }
        }

        // failed to export
        SKR_LOG_FMT_ERROR(
            u8"failed to export field {}::{}",
            owner_type->name(),
            field->name
        );
    });

    return result;
}
V8MatchSuggestion V8Matcher::_suggest_wrap(rttr::Type* type)
{
    if (flag_all(type->record_flag(), rttr::ERecordFlag::ScriptVisible))
    {
        if (type->based_on(rttr::type_id_of<rttr::ScriptbleObject>()))
        {
            return V8MatchSuggestion::FromWrap(type);
        }
    }

    return {};
}
bool V8Matcher::_match_primitive(const V8MatchSuggestion& suggestion, v8::Local<v8::Value> v8_value)
{
    switch (suggestion.primitive().type_id.get_hash())
    {
    case rttr::type_id_of<int8_t>().get_hash():
        return v8_value->IsInt32();
    case rttr::type_id_of<int16_t>().get_hash():
        return v8_value->IsInt32();
    case rttr::type_id_of<int32_t>().get_hash():
        return v8_value->IsInt32();
    case rttr::type_id_of<int64_t>().get_hash():
        return v8_value->IsBigInt();
    case rttr::type_id_of<uint8_t>().get_hash():
        return v8_value->IsUint32();
    case rttr::type_id_of<uint16_t>().get_hash():
        return v8_value->IsUint32();
    case rttr::type_id_of<uint32_t>().get_hash():
        return v8_value->IsUint32();
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
        SKR_UNREACHABLE_CODE()
        return false;
    }
}
bool V8Matcher::_match_box(const V8MatchSuggestion& suggestion, v8::Local<v8::Value> v8_value)
{
    auto isolate = v8::Isolate::GetCurrent();
    auto context = isolate->GetCurrentContext();

    if (!v8_value->IsObject()) { return false; }

    auto& box_data = suggestion.box();
    auto  v8_obj   = v8_value->ToObject(context).ToLocalChecked();

    // match primitive
    for (const auto& field_data : box_data.primitive_members)
    {
        // clang-format off
        auto v8_field_value = v8_obj->Get(
            context,
            V8BindTools::to_v8(field_data.field->name, true)
        );
        // clang-format on

        if (v8_field_value.IsEmpty()) { return false; }

        auto v8_field_value_solved = v8_field_value.ToLocalChecked();
        if (!_match_primitive(field_data.suggestion, v8_field_value_solved))
        {
            return false;
        }
    }

    // match box
    for (const auto& field_data : box_data.box_members)
    {
        // clang-format off
        auto v8_field_value = v8_obj->Get(
            context,
            V8BindTools::to_v8(field_data.field->name, true)
        );
        // clang-format on

        if (v8_field_value.IsEmpty()) { return false; }

        auto v8_field_value_solved = v8_field_value.ToLocalChecked();
        if (!_match_box(field_data.suggestion, v8_field_value_solved))
        {
            return false;
        }
    }

    return false;
}
bool V8Matcher::_match_wrap(const V8MatchSuggestion& suggestion, v8::Local<v8::Value> v8_value)
{
    auto isolate = v8::Isolate::GetCurrent();
    auto context = isolate->GetCurrentContext();

    auto& wrap_data = suggestion.wrap();
    auto* type      = wrap_data.type;

    // check v8 value type
    if (v8_value->IsObject())
    {
        // check wrap
        auto v8_obj = v8_value->ToObject(context).ToLocalChecked();
        if (v8_obj->InternalFieldCount() >= 1)
        {
            void*             raw_bind_core = v8_obj->GetInternalField(0).As<v8::External>()->Value();
            V8BindRecordCore* bind_core     = reinterpret_cast<V8BindRecordCore*>(raw_bind_core);

            // check inherit
            if (bind_core->type->based_on(type->type_id()))
            {
                return true;
            }
        }
    }

    return false;
}

// convert helpers
v8::Local<v8::Value> V8Matcher::_to_v8_primitive(
    const V8MatchSuggestion& suggestion,
    void*                    native_data
)
{
    switch (suggestion.primitive().type_id.get_hash())
    {
    case rttr::type_id_of<int8_t>().get_hash():
        return V8BindTools::to_v8(*reinterpret_cast<int8_t*>(native_data));
    case rttr::type_id_of<int16_t>().get_hash():
        return V8BindTools::to_v8(*reinterpret_cast<int16_t*>(native_data));
    case rttr::type_id_of<int32_t>().get_hash():
        return V8BindTools::to_v8(*reinterpret_cast<int32_t*>(native_data));
    case rttr::type_id_of<int64_t>().get_hash():
        return V8BindTools::to_v8(*reinterpret_cast<int64_t*>(native_data));
    case rttr::type_id_of<uint8_t>().get_hash():
        return V8BindTools::to_v8(*reinterpret_cast<uint8_t*>(native_data));
    case rttr::type_id_of<uint16_t>().get_hash():
        return V8BindTools::to_v8(*reinterpret_cast<uint16_t*>(native_data));
    case rttr::type_id_of<uint32_t>().get_hash():
        return V8BindTools::to_v8(*reinterpret_cast<uint32_t*>(native_data));
    case rttr::type_id_of<uint64_t>().get_hash():
        return V8BindTools::to_v8(*reinterpret_cast<uint64_t*>(native_data));
    case rttr::type_id_of<float>().get_hash():
        return V8BindTools::to_v8(*reinterpret_cast<float*>(native_data));
    case rttr::type_id_of<double>().get_hash():
        return V8BindTools::to_v8(*reinterpret_cast<double*>(native_data));
    case rttr::type_id_of<bool>().get_hash():
        return V8BindTools::to_v8(*reinterpret_cast<bool*>(native_data));
    case rttr::type_id_of<skr::String>().get_hash():
        return V8BindTools::to_v8(*reinterpret_cast<skr::String*>(native_data));
    default:
        SKR_UNREACHABLE_CODE()
        return {};
    }
}
bool V8Matcher::_to_native_primitive(
    const V8MatchSuggestion& suggestion,
    v8::Local<v8::Value>     v8_value,
    void*                    native_data,
    bool                     is_init
)
{
    switch (suggestion.primitive().type_id.get_hash())
    {
    case rttr::type_id_of<int8_t>().get_hash():
        return V8BindTools::to_native(v8_value, *reinterpret_cast<int8_t*>(native_data));
    case rttr::type_id_of<int16_t>().get_hash():
        return V8BindTools::to_native(v8_value, *reinterpret_cast<int16_t*>(native_data));
    case rttr::type_id_of<int32_t>().get_hash():
        return V8BindTools::to_native(v8_value, *reinterpret_cast<int32_t*>(native_data));
    case rttr::type_id_of<int64_t>().get_hash():
        return V8BindTools::to_native(v8_value, *reinterpret_cast<int64_t*>(native_data));
    case rttr::type_id_of<uint8_t>().get_hash():
        return V8BindTools::to_native(v8_value, *reinterpret_cast<uint8_t*>(native_data));
    case rttr::type_id_of<uint16_t>().get_hash():
        return V8BindTools::to_native(v8_value, *reinterpret_cast<uint16_t*>(native_data));
    case rttr::type_id_of<uint32_t>().get_hash():
        return V8BindTools::to_native(v8_value, *reinterpret_cast<uint32_t*>(native_data));
    case rttr::type_id_of<uint64_t>().get_hash():
        return V8BindTools::to_native(v8_value, *reinterpret_cast<uint64_t*>(native_data));
    case rttr::type_id_of<float>().get_hash():
        return V8BindTools::to_native(v8_value, *reinterpret_cast<float*>(native_data));
    case rttr::type_id_of<double>().get_hash():
        return V8BindTools::to_native(v8_value, *reinterpret_cast<double*>(native_data));
    case rttr::type_id_of<bool>().get_hash():
        return V8BindTools::to_native(v8_value, *reinterpret_cast<bool*>(native_data));
    case rttr::type_id_of<skr::String>().get_hash():
        if (!is_init)
        {
            new (native_data) skr::String();
        }
        return V8BindTools::to_native(v8_value, *reinterpret_cast<skr::String*>(native_data));
    default:
        SKR_UNREACHABLE_CODE()
        return false;
    }
}
v8::Local<v8::Value> V8Matcher::_to_v8_box(
    const V8MatchSuggestion& suggestion,
    void*                    native_data
)
{
    auto isolate = v8::Isolate::GetCurrent();
    auto context = isolate->GetCurrentContext();

    auto& box_data = suggestion.box();
    auto* type     = box_data.type;

    auto result = v8::Object::New(isolate);

    // primitive
    for (const auto& data : box_data.primitive_members)
    {
        // get field info
        auto  field               = data.field;
        auto  field_owner         = data.field_owner;
        void* field_owner_address = type->cast_to_base(field_owner->type_id(), native_data);
        void* field_address       = field->get_address(field_owner_address);

        // set result
        // clang-format off
        result->Set(
            context,
            V8BindTools::to_v8(field->name, true),
            conv_to_v8(data.suggestion, field_address)
        ).Check();
        // clang-format on
    }

    // box
    for (const auto& data : box_data.box_members)
    {
        // get field info
        auto  field               = data.field;
        auto  field_owner         = data.field_owner;
        void* field_owner_address = type->cast_to_base(field_owner->type_id(), native_data);
        void* field_address       = field->get_address(field_owner_address);

        // set result
        // clang-format off
        result->Set(
            context,
            V8BindTools::to_v8(field->name, true),
            conv_to_v8(data.suggestion, field_address)
        ).Check();
        // clang-format on
    }

    return result;
}
bool V8Matcher::_to_native_box(
    const V8MatchSuggestion& suggestion,
    v8::Local<v8::Value>     v8_value,
    void*                    native_data,
    bool                     is_init
)
{
    auto isolate = v8::Isolate::GetCurrent();
    auto context = isolate->GetCurrentContext();

    auto& box_data  = suggestion.box();
    auto* type      = box_data.type;
    auto  v8_object = v8_value->ToObject(context).ToLocalChecked();

    // do init
    if (!is_init)
    {
        void* raw_invoker = type->find_ctor_t<void()>()->native_invoke;
        auto* invoker     = reinterpret_cast<void (*)(void*)>(raw_invoker);
        invoker(native_data);
    }

    // primitive
    for (const auto& data : box_data.primitive_members)
    {
        // get field info
        auto  field               = data.field;
        auto  field_owner         = data.field_owner;
        void* field_owner_address = type->cast_to_base(field_owner->type_id(), native_data);
        void* field_address       = field->get_address(field_owner_address);

        // find object field
        // clang-format off
        auto v8_field = v8_object->Get(
            context,
            V8BindTools::str_to_v8(field->name, isolate)
        ).ToLocalChecked();
        // clang-format on

        // to native
        conv_to_native(data.suggestion, field_address, v8_field, true);
    }

    // box
    for (const auto& data : box_data.box_members)
    {
        // get field info
        auto  field               = data.field;
        auto  field_owner         = data.field_owner;
        void* field_owner_address = type->cast_to_base(field_owner->type_id(), native_data);
        void* field_address       = field->get_address(field_owner_address);

        // find object field
        // clang-format off
        auto v8_field = v8_object->Get(
            context,
            V8BindTools::str_to_v8(field->name, isolate)
        ).ToLocalChecked();
        // clang-format on

        // to native
        conv_to_native(data.suggestion, field_address, v8_field, true);
    }

    return true;
}
::v8::Local<::v8::Value> V8Matcher::_to_v8_wrap(
    const V8MatchSuggestion& suggestion,
    void*                    native_data
)
{
    auto isolate     = v8::Isolate::GetCurrent();
    auto skr_isolate = reinterpret_cast<V8Isolate*>(isolate->GetData(0));

    auto& wrap_data        = suggestion.wrap();
    auto* type             = wrap_data.type;
    void* cast_raw         = type->cast_to_base(rttr::type_id_of<rttr::ScriptbleObject>(), native_data);
    auto* scriptble_object = reinterpret_cast<rttr::ScriptbleObject*>(cast_raw);

    auto* bind_core = skr_isolate->translate_record(scriptble_object);
    if (bind_core)
    {
        return bind_core->v8_object.Get(isolate);
    }

    return {};
}
bool V8Matcher::_to_native_wrap(
    const V8MatchSuggestion& suggestion,
    ::v8::Local<::v8::Value> v8_value,
    void*                    native_data,
    bool                     is_init
)
{
    auto isolate     = v8::Isolate::GetCurrent();
    auto context     = isolate->GetCurrentContext();
    auto skr_isolate = reinterpret_cast<V8Isolate*>(isolate->GetData(0));

    auto& wrap_data = suggestion.wrap();
    auto* type      = wrap_data.type;

    if (v8_value->IsObject())
    {
        auto v8_object = v8_value->ToObject(context).ToLocalChecked();
        if (v8_object->InternalFieldCount() >= 1)
        {
            void* raw_bind_core = v8_object->GetInternalField(0).As<v8::External>()->Value();
            auto* bind_core     = reinterpret_cast<V8BindRecordCore*>(raw_bind_core);

            // do cast
            void* cast_ptr                         = bind_core->cast_to_base(type->type_id());
            *reinterpret_cast<void**>(native_data) = cast_ptr;
            return true;
        }
    }

    return false;
}
} // namespace skr