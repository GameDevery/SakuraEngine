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
        result = _suggest_wrap(type);
        if (!result.is_empty())
        {
            // check v8 value type
            if (v8_value->IsObject())
            {
                // check wrap data
                auto v8_obj = v8_value->ToObject(context).ToLocalChecked();
                if (v8_obj->InternalFieldCount() >= 1)
                {
                    void*             raw_bind_core = v8_obj->GetInternalField(0).As<v8::External>()->Value();
                    V8BindRecordCore* bind_core     = reinterpret_cast<V8BindRecordCore*>(raw_bind_core);
                    
                    // check inherit
                    if (bind_core->type->based_on(type->type_id()))
                    {
                        return result;
                    }
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
        result = _suggest_wrap(type);
        if (!result.is_empty()) { return result; }
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
    return {};
}
bool V8Matcher::conv_to_native(
    const V8MatchSuggestion& suggestion,
    void*                    native_data,
    ::v8::Local<::v8::Value> v8_value,
    bool                     is_init
)
{
    return false;
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
} // namespace skr