#pragma once
#include "SkrRTTR/scriptble_object.hpp"
#include "SkrRTTR/script_binder.hpp"
#include "v8_isolate.hpp"

// to v8
namespace skr::v8_bind
{
inline static v8::Local<v8::Value> to_v8(int32_t v)
{
    auto isolate = v8::Isolate::GetCurrent();
    return v8::Integer::New(isolate, v);
}
inline static v8::Local<v8::Value> to_v8(int64_t v)
{
    auto isolate = v8::Isolate::GetCurrent();
    return v8::BigInt::New(isolate, v);
}
inline static v8::Local<v8::Value> to_v8(uint32_t v)
{
    auto isolate = v8::Isolate::GetCurrent();
    return v8::Integer::NewFromUnsigned(isolate, v);
}
inline static v8::Local<v8::Value> to_v8(uint64_t v)
{
    auto isolate = v8::Isolate::GetCurrent();
    return v8::BigInt::NewFromUnsigned(isolate, v);
}
inline static v8::Local<v8::Value> to_v8(double v)
{
    auto isolate = v8::Isolate::GetCurrent();
    return v8::Number::New(isolate, v);
}
inline static v8::Local<v8::Value> to_v8(bool v)
{
    auto isolate = v8::Isolate::GetCurrent();
    return v8::Boolean::New(isolate, v);
}
inline static v8::Local<v8::String> to_v8(StringView view, bool as_literal = false)
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
} // namespace skr::v8_bind

// to native
namespace skr::v8_bind
{
inline static bool to_native(v8::Local<v8::Value> v8_value, int8_t& out_v)
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
inline static bool to_native(v8::Local<v8::Value> v8_value, int16_t& out_v)
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
inline static bool to_native(v8::Local<v8::Value> v8_value, int32_t& out_v)
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
inline static bool to_native(v8::Local<v8::Value> v8_value, int64_t& out_v)
{
    auto isolate = v8::Isolate::GetCurrent();
    auto context = isolate->GetCurrentContext();

    if (v8_value->IsBigInt())
    {
        out_v = (int64_t)v8_value->ToBigInt(context).ToLocalChecked()->Int64Value();
        return true;
    }
    return false;
}
inline static bool to_native(v8::Local<v8::Value> v8_value, uint8_t& out_v)
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
inline static bool to_native(v8::Local<v8::Value> v8_value, uint16_t& out_v)
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
inline static bool to_native(v8::Local<v8::Value> v8_value, uint32_t& out_v)
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
inline static bool to_native(v8::Local<v8::Value> v8_value, uint64_t& out_v)
{
    auto isolate = v8::Isolate::GetCurrent();
    auto context = isolate->GetCurrentContext();

    if (v8_value->IsBigInt())
    {
        out_v = (uint64_t)v8_value->ToBigInt(context).ToLocalChecked()->Uint64Value();
        return true;
    }
    return false;
}
inline static bool to_native(v8::Local<v8::Value> v8_value, float& out_v)
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
inline static bool to_native(v8::Local<v8::Value> v8_value, double& out_v)
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
inline static bool to_native(v8::Local<v8::Value> v8_value, bool& out_v)
{
    auto isolate = v8::Isolate::GetCurrent();

    if (v8_value->IsBoolean())
    {
        out_v = v8_value->BooleanValue(isolate);
        return true;
    }
    return false;
}
inline static bool to_native(v8::Local<v8::Value> v8_value, skr::String& out_v)
{
    auto isolate = v8::Isolate::GetCurrent();

    if (v8_value->IsString())
    {
        out_v = skr::String::From(*v8::String::Utf8Value(isolate, v8_value));
        return true;
    }
    return false;
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
);
bool set_field(
    const ScriptBinderStaticField& binder,
    v8::Local<v8::Value>           v8_value
);
v8::Local<v8::Value> get_field(
    const ScriptBinderField& binder,
    const void*              obj,
    const RTTRType*          obj_type
);
v8::Local<v8::Value> get_field(
    const ScriptBinderStaticField& binder
);

// call native
bool call_native(
    const ScriptBinderCtor&                        binder,
    const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack,
    MethodInvokerDynamicStack                      invoker,
    void*                                          obj
);
bool call_native(
    const ScriptBinderMethod&                      binder,
    const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack,
    MethodInvokerDynamicStack                      invoker,
    void*                                          obj
);
bool call_native(
    const ScriptBinderStaticMethod&                binder,
    const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack,
    FuncInvokerDynamicStack                        invoker
);
} // namespace skr::v8_bind

// raw convert and match
namespace skr::v8_bind
{
// convert
v8::Local<v8::Value> to_v8(
    ScriptBinderRoot binder,
    void*            native_data
);
bool to_native(
    ScriptBinderRoot     binder,
    void*                native_data,
    v8::Local<v8::Value> v8_value,
    bool                 is_init
);

// match type
bool match(
    ScriptBinderRoot     binder,
    v8::Local<v8::Value> v8_value
);
bool match(
    const ScriptBinderParam& binder,
    v8::Local<v8::Value>     v8_value
);
bool match(
    const ScriptBinderReturn& binder,
    v8::Local<v8::Value>      v8_value
);
bool match(
    const ScriptBinderField& binder,
    v8::Local<v8::Value>     v8_value
);
bool match(
    const ScriptBinderStaticField& binder,
    v8::Local<v8::Value>           v8_value
);
bool match(
    const Vector<ScriptBinderParam>&               param_binders,
    const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack
);
} // namespace skr::v8_bind