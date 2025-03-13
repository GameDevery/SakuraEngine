#include "SkrV8/v8_matcher.hpp"
#include "SkrV8/v8_bind_tools.hpp"

// helpers
namespace skr
{
    inline static ::v8::Local<::v8::Value> _to_v8_primitive(
        const MatchSuggestion& suggestion,
        void*                  native_data
    )
    {
        switch (suggestion.primitive.type_id.get_hash())
        {
        case rttr::type_id_of<int8_t>().get_hash():
            return v8::V8BindTools::to_v8(*reinterpret_cast<int8_t*>(native_data));
        case rttr::type_id_of<int16_t>().get_hash():
            return v8::V8BindTools::to_v8(*reinterpret_cast<int16_t*>(native_data));
        case rttr::type_id_of<int32_t>().get_hash():
            return v8::V8BindTools::to_v8(*reinterpret_cast<int32_t*>(native_data));
        case rttr::type_id_of<int64_t>().get_hash():
            return v8::V8BindTools::to_v8(*reinterpret_cast<int64_t*>(native_data));
        case rttr::type_id_of<uint8_t>().get_hash():
            return v8::V8BindTools::to_v8(*reinterpret_cast<uint8_t*>(native_data));
        case rttr::type_id_of<uint16_t>().get_hash():
            return v8::V8BindTools::to_v8(*reinterpret_cast<uint16_t*>(native_data));
        case rttr::type_id_of<uint32_t>().get_hash():
            return v8::V8BindTools::to_v8(*reinterpret_cast<uint32_t*>(native_data));
        case rttr::type_id_of<uint64_t>().get_hash():
            return v8::V8BindTools::to_v8(*reinterpret_cast<uint64_t*>(native_data));
        case rttr::type_id_of<float>().get_hash():
            return v8::V8BindTools::to_v8(*reinterpret_cast<float*>(native_data));
        case rttr::type_id_of<double>().get_hash():
            return v8::V8BindTools::to_v8(*reinterpret_cast<double*>(native_data));
        case rttr::type_id_of<bool>().get_hash():
            return v8::V8BindTools::to_v8(*reinterpret_cast<bool*>(native_data));
        case rttr::type_id_of<skr::String>().get_hash():
            return v8::V8BindTools::to_v8(*reinterpret_cast<skr::String*>(native_data));
        default:
            SKR_UNREACHABLE_CODE()
            return {};
        }
    }
    inline static bool _to_native_primitive(
        const MatchSuggestion&   suggestion,
        ::v8::Local<::v8::Value> v8_value,
        void*                    native_data,
        bool                     is_init
    )
    {
        switch (suggestion.primitive.type_id.get_hash())
        {
        case rttr::type_id_of<int8_t>().get_hash():
            return v8::V8BindTools::to_native(v8_value, *reinterpret_cast<int8_t*>(native_data));
        case rttr::type_id_of<int16_t>().get_hash():
            return v8::V8BindTools::to_native(v8_value, *reinterpret_cast<int16_t*>(native_data));
        case rttr::type_id_of<int32_t>().get_hash():
            return v8::V8BindTools::to_native(v8_value, *reinterpret_cast<int32_t*>(native_data));
        case rttr::type_id_of<int64_t>().get_hash():
            return v8::V8BindTools::to_native(v8_value, *reinterpret_cast<int64_t*>(native_data));
        case rttr::type_id_of<uint8_t>().get_hash():
            return v8::V8BindTools::to_native(v8_value, *reinterpret_cast<uint8_t*>(native_data));
        case rttr::type_id_of<uint16_t>().get_hash():
            return v8::V8BindTools::to_native(v8_value, *reinterpret_cast<uint16_t*>(native_data));
        case rttr::type_id_of<uint32_t>().get_hash():
            return v8::V8BindTools::to_native(v8_value, *reinterpret_cast<uint32_t*>(native_data));
        case rttr::type_id_of<uint64_t>().get_hash():
            return v8::V8BindTools::to_native(v8_value, *reinterpret_cast<uint64_t*>(native_data));
        case rttr::type_id_of<float>().get_hash():
            return v8::V8BindTools::to_native(v8_value, *reinterpret_cast<float*>(native_data));
        case rttr::type_id_of<double>().get_hash():
            return v8::V8BindTools::to_native(v8_value, *reinterpret_cast<double*>(native_data));
        case rttr::type_id_of<bool>().get_hash():
            return v8::V8BindTools::to_native(v8_value, *reinterpret_cast<bool*>(native_data));
        case rttr::type_id_of<skr::String>().get_hash():
            if (!is_init)
            {
                new (native_data) skr::String();
            }
            return v8::V8BindTools::to_native(v8_value, *reinterpret_cast<skr::String*>(native_data));
        default:
            SKR_UNREACHABLE_CODE()
            return false;
        }
    }
    inline static ::v8::Local<::v8::Value> _to_v8_box(
        const MatchSuggestion& suggestion,
        void*                  native_data
    )
    {
        auto isolate = ::v8::Isolate::GetCurrent();
        auto context = isolate->GetCurrentContext();
    
        auto* type = suggestion.box.type;
    
        auto result = ::v8::Object::New(isolate);
    
        // primitive
        for (const auto& data: suggestion.box.primitive_members)
        {
            // get field info
            auto  field               = data.field;
            auto  field_owner         = data.field_owner;
            void* field_owner_address = type->cast_to_base(field_owner->type_id(), native_data);
            void* field_address       = field->get_address(field_owner_address);
    
            // set result
            result->Set(
                context,
                v8::V8BindTools::to_v8(field->name, true),
                Matcher::conv_to_v8(data.suggestion, field_address)
            ).Check();
        }
    
        // box
        for (const auto& data: suggestion.box.box_members)
        {
            // get field info
            auto  field               = data.field;
            auto  field_owner         = data.field_owner;
            void* field_owner_address = type->cast_to_base(field_owner->type_id(), native_data);
            void* field_address       = field->get_address(field_owner_address);
    
            // set result
            result->Set(
                context,
                v8::V8BindTools::to_v8(field->name, true),
                Matcher::conv_to_v8(data.suggestion, field_address)
            ).Check();
        }
    
        return result;
    }
    inline static bool _to_native_box(
        const MatchSuggestion&   suggestion,
        ::v8::Local<::v8::Value> v8_value,
        void*                    native_data,
        bool                     is_init
    )
    {
        auto isolate = ::v8::Isolate::GetCurrent();
        auto context = isolate->GetCurrentContext();
    
        auto* type = suggestion.box.type;
        auto v8_object = v8_value->ToObject(context).ToLocalChecked();
    
        // do init
        if (!is_init)
        {
            void* raw_invoker = type->find_ctor_t<void()>()->native_invoke;
            auto* invoker     = reinterpret_cast<void (*)(void*)>(raw_invoker);
            invoker(native_data);
        }
    
        // primitive
        for (const auto& data: suggestion.box.primitive_members)
        {
            // get field info
            auto  field               = data.field;
            auto  field_owner         = data.field_owner;
            void* field_owner_address = type->cast_to_base(field_owner->type_id(), native_data);
            void* field_address       = field->get_address(field_owner_address);
    
            // find object field
            auto v8_field = v8_object->Get(
                context,
                v8::V8BindTools::str_to_v8(field->name, isolate)
            ).ToLocalChecked();
    
            // to native
            Matcher::conv_to_native(data.suggestion, field_address, v8_field, true);
        }
    
        // box
        for (const auto& data: suggestion.box.box_members)
        {
            // get field info
            auto  field               = data.field;
            auto  field_owner         = data.field_owner;
            void* field_owner_address = type->cast_to_base(field_owner->type_id(), native_data);
            void* field_address       = field->get_address(field_owner_address);
    
            // find object field
            auto v8_field = v8_object->Get(
                context,
                v8::V8BindTools::str_to_v8(field->name, isolate)
            ).ToLocalChecked();
    
            // to native
            Matcher::conv_to_native(data.suggestion, field_address, v8_field, true);
        }
    
        return true;
    }
}