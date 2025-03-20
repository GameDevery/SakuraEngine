#pragma once
#include "SkrRTTR/scriptble_object.hpp"
#include "SkrRTTR/script_binder.hpp"
#include "v8_isolate.hpp"

namespace skr
{
// TODO. _to_v8/_to_native value
// TODO. field 需要特殊处理
struct V8Bind {
    // primitive to v8
    static v8::Local<v8::Value>  to_v8(int32_t v);
    static v8::Local<v8::Value>  to_v8(int64_t v);
    static v8::Local<v8::Value>  to_v8(uint32_t v);
    static v8::Local<v8::Value>  to_v8(uint64_t v);
    static v8::Local<v8::Value>  to_v8(double v);
    static v8::Local<v8::Value>  to_v8(bool v);
    static v8::Local<v8::String> to_v8(StringView view, bool as_literal = false);

    // primitive to native
    static bool to_native(v8::Local<v8::Value> v8_value, int8_t& out_v);
    static bool to_native(v8::Local<v8::Value> v8_value, int16_t& out_v);
    static bool to_native(v8::Local<v8::Value> v8_value, int32_t& out_v);
    static bool to_native(v8::Local<v8::Value> v8_value, int64_t& out_v);
    static bool to_native(v8::Local<v8::Value> v8_value, uint8_t& out_v);
    static bool to_native(v8::Local<v8::Value> v8_value, uint16_t& out_v);
    static bool to_native(v8::Local<v8::Value> v8_value, uint32_t& out_v);
    static bool to_native(v8::Local<v8::Value> v8_value, uint64_t& out_v);
    static bool to_native(v8::Local<v8::Value> v8_value, float& out_v);
    static bool to_native(v8::Local<v8::Value> v8_value, double& out_v);
    static bool to_native(v8::Local<v8::Value> v8_value, bool& out_v);
    static bool to_native(v8::Local<v8::Value> v8_value, skr::String& out_v);

    // enum convert
    static v8::Local<v8::Value> to_v8(EnumValue value);

    // field tools
    static bool set_field(
        const ScriptBinderField& binder,
        v8::Local<v8::Value>     v8_value,
        V8BindCoreRecordBase*    bind_core
    );
    static bool set_field(
        const ScriptBinderField& binder,
        v8::Local<v8::Value>     v8_value,
        void*                    obj,
        const RTTRType*          obj_type
    );
    static bool set_field(
        const ScriptBinderStaticField& binder,
        v8::Local<v8::Value>           v8_value
    );
    static v8::Local<v8::Value> get_field(
        const ScriptBinderField& binder,
        V8BindCoreRecordBase*    bind_core
    );
    static v8::Local<v8::Value> get_field(
        const ScriptBinderField& binder,
        const void*              obj,
        const RTTRType*          obj_type
    );
    static v8::Local<v8::Value> get_field(
        const ScriptBinderStaticField& binder
    );

    // call native
    static bool call_native(
        const ScriptBinderCtor&                        binder,
        const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack,
        void*                                          obj
    );
    static bool call_native(
        const ScriptBinderMethod&                      binder,
        const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack,
        void*                                          obj,
        const RTTRType*                                obj_type
    );
    static bool call_native(
        const ScriptBinderStaticMethod&                binder,
        const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack
    );

    // convert
    static v8::Local<v8::Value> to_v8(
        ScriptBinderRoot binder,
        void*            native_data
    );
    static bool to_native(
        ScriptBinderRoot     binder,
        void*                native_data,
        v8::Local<v8::Value> v8_value,
        bool                 is_init,
        bool                 pass_by_ref
    );

    // match type
    static bool match(
        ScriptBinderRoot     binder,
        v8::Local<v8::Value> v8_value
    );
    static bool match(
        const ScriptBinderParam& binder,
        v8::Local<v8::Value>     v8_value
    );
    static bool match(
        const ScriptBinderReturn& binder,
        v8::Local<v8::Value>      v8_value
    );
    static bool match(
        const ScriptBinderField& binder,
        v8::Local<v8::Value>     v8_value
    );
    static bool match(
        const ScriptBinderStaticField& binder,
        v8::Local<v8::Value>           v8_value
    );
    static bool match(
        const Vector<ScriptBinderParam>&               param_binders,
        uint32_t                                       solved_param_count,
        const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack
    );

private:
    // util helpers
    static void* _get_field_address(
        const RTTRFieldData* field,
        const RTTRType*      field_owner,
        const RTTRType*      obj_type,
        void*                obj
    );

    // match helper
    static bool _match_primitive(const ScriptBinderPrimitive& binder, v8::Local<v8::Value> v8_value);
    static bool _match_mapping(const ScriptBinderMapping& binder, v8::Local<v8::Value> v8_value);
    static bool _match_object(const ScriptBinderObject& binder, v8::Local<v8::Value> v8_value);
    static bool _match_value(const ScriptBinderValue& binder, v8::Local<v8::Value> v8_value);

    // convert helper
    static v8::Local<v8::Value> _to_v8_primitive(
        const ScriptBinderPrimitive& binder,
        void*                        native_data
    );
    static void _init_primitive(
        const ScriptBinderPrimitive& binder,
        void*                        native_data
    );
    static bool _to_native_primitive(
        const ScriptBinderPrimitive& binder,
        v8::Local<v8::Value>         v8_value,
        void*                        native_data,
        bool                         is_init
    );
    static v8::Local<v8::Value> _to_v8_mapping(
        const ScriptBinderMapping& binder,
        void*                      obj
    );
    static bool _to_native_mapping(
        const ScriptBinderMapping& binder,
        v8::Local<v8::Value>       v8_value,
        void*                      native_data,
        bool                       is_init
    );
    static v8::Local<v8::Value> _to_v8_object(
        const ScriptBinderObject& binder,
        void*                     native_data
    );
    static bool _to_native_object(
        const ScriptBinderObject& binder,
        v8::Local<v8::Value>      v8_value,
        void*                     native_data,
        bool                      is_init
    );
    static v8::Local<v8::Value> _to_v8_value(
        const ScriptBinderValue& binder,
        void*                    native_data
    );
    static bool _to_native_value(
        const ScriptBinderValue& binder,
        v8::Local<v8::Value>     v8_value,
        void*                    native_data,
        bool                     is_init,
        bool                     pass_by_ref
    );

    // invoker helper
    static void _push_param(
        DynamicStack&            stack,
        const ScriptBinderParam& param_binder,
        v8::Local<v8::Value>     v8_value
    );
    static void _push_param_pure_out(
        DynamicStack&            stack,
        const ScriptBinderParam& param_binder
    );
    static v8::Local<v8::Value> _read_return(
        DynamicStack&                    stack,
        const Vector<ScriptBinderParam>& params_binder,
        const ScriptBinderReturn&        return_binder,
        uint32_t                         solved_return_count
    );
    static v8::Local<v8::Value> _read_return(
        DynamicStack&             stack,
        const ScriptBinderReturn& return_binder
    );
    static v8::Local<v8::Value> _read_return_from_out_param(
        DynamicStack&            stack,
        const ScriptBinderParam& param_binder
    );
};
} // namespace skr

// to v8
namespace skr
{
inline v8::Local<v8::Value> V8Bind::to_v8(int32_t v)
{
    auto isolate = v8::Isolate::GetCurrent();
    return v8::Integer::New(isolate, v);
}
inline v8::Local<v8::Value> V8Bind::to_v8(int64_t v)
{
    auto isolate = v8::Isolate::GetCurrent();
    return v8::BigInt::New(isolate, v);
}
inline v8::Local<v8::Value> V8Bind::to_v8(uint32_t v)
{
    auto isolate = v8::Isolate::GetCurrent();
    return v8::Integer::NewFromUnsigned(isolate, v);
}
inline v8::Local<v8::Value> V8Bind::to_v8(uint64_t v)
{
    auto isolate = v8::Isolate::GetCurrent();
    return v8::BigInt::NewFromUnsigned(isolate, v);
}
inline v8::Local<v8::Value> V8Bind::to_v8(double v)
{
    auto isolate = v8::Isolate::GetCurrent();
    return v8::Number::New(isolate, v);
}
inline v8::Local<v8::Value> V8Bind::to_v8(bool v)
{
    auto isolate = v8::Isolate::GetCurrent();
    return v8::Boolean::New(isolate, v);
}
inline v8::Local<v8::String> V8Bind::to_v8(StringView view, bool as_literal)
{
    auto isolate = v8::Isolate::GetCurrent();
    // clang-format off
    return v8::String::NewFromUtf8(
        isolate,
        view.data_raw(),
        as_literal ? v8::NewStringType::kInternalized : v8::NewStringType::kNormal,
        view.length_buffer()
    ).ToLocalChecked();
    // clang-format on
}
} // namespace skr

// to native
namespace skr
{
inline bool V8Bind::to_native(v8::Local<v8::Value> v8_value, int8_t& out_v)
{
    auto isolate = v8::Isolate::GetCurrent();
    auto context = isolate->GetCurrentContext();

    if (v8_value->IsInt32())
    {
        out_v = (int8_t)v8_value->Int32Value(context).ToChecked();
        return true;
    }
    return false;
}
inline bool V8Bind::to_native(v8::Local<v8::Value> v8_value, int16_t& out_v)
{
    auto isolate = v8::Isolate::GetCurrent();
    auto context = isolate->GetCurrentContext();

    if (v8_value->IsInt32())
    {
        out_v = (int16_t)v8_value->Int32Value(context).ToChecked();
        return true;
    }
    return false;
}
inline bool V8Bind::to_native(v8::Local<v8::Value> v8_value, int32_t& out_v)
{
    auto isolate = v8::Isolate::GetCurrent();
    auto context = isolate->GetCurrentContext();

    if (v8_value->IsInt32())
    {
        out_v = (int32_t)v8_value->Int32Value(context).ToChecked();
        return true;
    }
    return false;
}
inline bool V8Bind::to_native(v8::Local<v8::Value> v8_value, int64_t& out_v)
{
    auto isolate = v8::Isolate::GetCurrent();
    auto context = isolate->GetCurrentContext();

    if (v8_value->IsBigInt())
    {
        out_v = (int64_t)v8_value->ToBigInt(context).ToLocalChecked()->Int64Value();
        return true;
    }
    else if (v8_value->IsInt32())
    {
        out_v = (int64_t)v8_value->Int32Value(context).ToChecked();
        return true;
    }
    else if (v8_value->IsUint32())
    {
        out_v = (int64_t)v8_value->Uint32Value(context).ToChecked();
        return true;
    }
    return false;
}
inline bool V8Bind::to_native(v8::Local<v8::Value> v8_value, uint8_t& out_v)
{
    auto isolate = v8::Isolate::GetCurrent();
    auto context = isolate->GetCurrentContext();

    if (v8_value->IsUint32())
    {
        out_v = (uint8_t)v8_value->Uint32Value(context).ToChecked();
        return true;
    }
    return false;
}
inline bool V8Bind::to_native(v8::Local<v8::Value> v8_value, uint16_t& out_v)
{
    auto isolate = v8::Isolate::GetCurrent();
    auto context = isolate->GetCurrentContext();

    if (v8_value->IsUint32())
    {
        out_v = (uint16_t)v8_value->Uint32Value(context).ToChecked();
        return true;
    }
    return false;
}
inline bool V8Bind::to_native(v8::Local<v8::Value> v8_value, uint32_t& out_v)
{
    auto isolate = v8::Isolate::GetCurrent();
    auto context = isolate->GetCurrentContext();

    if (v8_value->IsUint32())
    {
        out_v = (uint32_t)v8_value->Uint32Value(context).ToChecked();
        return true;
    }
    return false;
}
inline bool V8Bind::to_native(v8::Local<v8::Value> v8_value, uint64_t& out_v)
{
    auto isolate = v8::Isolate::GetCurrent();
    auto context = isolate->GetCurrentContext();

    if (v8_value->IsBigInt())
    {
        out_v = (uint64_t)v8_value->ToBigInt(context).ToLocalChecked()->Uint64Value();
        return true;
    }
    else if (v8_value->IsUint32())
    {
        out_v = (uint64_t)v8_value->Uint32Value(context).ToChecked();
        return true;
    }
    return false;
}
inline bool V8Bind::to_native(v8::Local<v8::Value> v8_value, float& out_v)
{
    auto isolate = v8::Isolate::GetCurrent();
    auto context = isolate->GetCurrentContext();

    if (v8_value->IsNumber())
    {
        out_v = (float)v8_value->NumberValue(context).ToChecked();
        return true;
    }
    return false;
}
inline bool V8Bind::to_native(v8::Local<v8::Value> v8_value, double& out_v)
{
    auto isolate = v8::Isolate::GetCurrent();
    auto context = isolate->GetCurrentContext();

    if (v8_value->IsNumber())
    {
        out_v = (double)v8_value->NumberValue(context).ToChecked();
        return true;
    }
    return false;
}
inline bool V8Bind::to_native(v8::Local<v8::Value> v8_value, bool& out_v)
{
    auto isolate = v8::Isolate::GetCurrent();

    if (v8_value->IsBoolean())
    {
        out_v = v8_value->BooleanValue(isolate);
        return true;
    }
    return false;
}
inline bool V8Bind::to_native(v8::Local<v8::Value> v8_value, skr::String& out_v)
{
    auto isolate = v8::Isolate::GetCurrent();

    if (v8_value->IsString())
    {
        out_v = skr::String::From(*v8::String::Utf8Value(isolate, v8_value));
        return true;
    }
    return false;
}

// enum convert
inline v8::Local<v8::Value> V8Bind::to_v8(EnumValue enum_value)
{
    auto isolate = v8::Isolate::GetCurrent();

    switch (enum_value.underlying_type())
    {
    case EEnumUnderlyingType::INT8:
    case EEnumUnderlyingType::INT16:
    case EEnumUnderlyingType::INT32: {
        int32_t value;
        SKR_ASSERT(enum_value.cast_to(value));
        return v8::Integer::New(isolate, value);
        break;
    }
    case EEnumUnderlyingType::UINT8:
    case EEnumUnderlyingType::UINT16:
    case EEnumUnderlyingType::UINT32: {
        uint32_t value;
        SKR_ASSERT(enum_value.cast_to(value));
        return v8::Integer::New(isolate, value);
        break;
    }
    case EEnumUnderlyingType::INT64: {
        int64_t value;
        SKR_ASSERT(enum_value.cast_to(value));
        return v8::BigInt::New(isolate, value);
        break;
    }
    case EEnumUnderlyingType::UINT64: {
        uint64_t value;
        SKR_ASSERT(enum_value.cast_to(value));
        return v8::BigInt::New(isolate, value);
        break;
    }
    default:
        SKR_UNREACHABLE_CODE()
        return {};
    }
}
} // namespace skr