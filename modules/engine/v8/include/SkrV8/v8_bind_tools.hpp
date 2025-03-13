#pragma once
#include "v8-template.h"
#include "SkrRTTR/type_signature.hpp"
#include "SkrRTTR/type.hpp"
#include "SkrRTTR/export/export_data.hpp"
#include "v8_isolate.hpp"

namespace skr
{
struct SKR_V8_API V8BindTools {
    // to v8
    inline static ::v8::Local<::v8::Value> to_v8(int32_t v)
    {
        auto isolate = ::v8::Isolate::GetCurrent();
        return ::v8::Integer::New(isolate, v);
    }
    inline static ::v8::Local<::v8::Value> to_v8(int64_t v)
    {
        auto isolate = ::v8::Isolate::GetCurrent();
        return ::v8::BigInt::New(isolate, v);
    }
    inline static ::v8::Local<::v8::Value> to_v8(uint32_t v)
    {
        auto isolate = ::v8::Isolate::GetCurrent();
        return ::v8::Integer::NewFromUnsigned(isolate, v);
    }
    inline static ::v8::Local<::v8::Value> to_v8(uint64_t v)
    {
        auto isolate = ::v8::Isolate::GetCurrent();
        return ::v8::BigInt::NewFromUnsigned(isolate, v);
    }
    inline static ::v8::Local<::v8::Value> to_v8(double v)
    {
        auto isolate = ::v8::Isolate::GetCurrent();
        return ::v8::Number::New(isolate, v);
    }
    inline static ::v8::Local<::v8::Value> to_v8(bool v)
    {
        auto isolate = ::v8::Isolate::GetCurrent();
        return ::v8::Boolean::New(isolate, v);
    }
    inline static ::v8::Local<::v8::String> to_v8(StringView view, bool as_literal = false) {
        auto isolate = ::v8::Isolate::GetCurrent();
        return ::v8::String::NewFromUtf8(
            isolate, 
            view.data_raw(), 
            as_literal ? ::v8::NewStringType::kInternalized : ::v8::NewStringType::kNormal, 
            view.length_buffer()
        ).ToLocalChecked();
    }

    // to native
    inline static bool to_native(::v8::Local<::v8::Value> v8_value, int8_t& out_v)
    {
        auto isolate = ::v8::Isolate::GetCurrent();
        auto context = isolate->GetCurrentContext();

        if (v8_value->IsInt32())
        {
            out_v = (int8_t)v8_value->Int32Value(context).ToChecked();
            return true;
        }
        return false;
    }
    inline static bool to_native(::v8::Local<::v8::Value> v8_value, int16_t& out_v)
    {
        auto isolate = ::v8::Isolate::GetCurrent();
        auto context = isolate->GetCurrentContext();

        if (v8_value->IsInt32())
        {
            out_v = (int16_t)v8_value->Int32Value(context).ToChecked();
            return true;
        }
        return false;
    }
    inline static bool to_native(::v8::Local<::v8::Value> v8_value, int32_t& out_v)
    {
        auto isolate = ::v8::Isolate::GetCurrent();
        auto context = isolate->GetCurrentContext();

        if (v8_value->IsInt32())
        {
            out_v = (int32_t)v8_value->Int32Value(context).ToChecked();
            return true;
        }
        return false;
    }
    inline static bool to_native(::v8::Local<::v8::Value> v8_value, int64_t& out_v)
    {
        auto isolate = ::v8::Isolate::GetCurrent();
        auto context = isolate->GetCurrentContext();

        if (v8_value->IsBigInt())
        {
            out_v = (int64_t)v8_value->ToBigInt(context).ToLocalChecked()->Int64Value();
            return true;
        }
        return false;
    }
    inline static bool to_native(::v8::Local<::v8::Value> v8_value, uint8_t& out_v)
    {
        auto isolate = ::v8::Isolate::GetCurrent();
        auto context = isolate->GetCurrentContext();

        if (v8_value->IsUint32())
        {
            out_v = (uint8_t)v8_value->Uint32Value(context).ToChecked();
            return true;
        }
        return false;
    }
    inline static bool to_native(::v8::Local<::v8::Value> v8_value, uint16_t& out_v)
    {
        auto isolate = ::v8::Isolate::GetCurrent();
        auto context = isolate->GetCurrentContext();

        if (v8_value->IsUint32())
        {
            out_v = (uint16_t)v8_value->Uint32Value(context).ToChecked();
            return true;
        }
        return false;
    }
    inline static bool to_native(::v8::Local<::v8::Value> v8_value, uint32_t& out_v)
    {
        auto isolate = ::v8::Isolate::GetCurrent();
        auto context = isolate->GetCurrentContext();

        if (v8_value->IsUint32())
        {
            out_v = (uint32_t)v8_value->Uint32Value(context).ToChecked();
            return true;
        }
        return false;
    }
    inline static bool to_native(::v8::Local<::v8::Value> v8_value, uint64_t& out_v)
    {
        auto isolate = ::v8::Isolate::GetCurrent();
        auto context = isolate->GetCurrentContext();

        if (v8_value->IsBigInt())
        {
            out_v = (uint64_t)v8_value->ToBigInt(context).ToLocalChecked()->Uint64Value();
            return true;
        }
        return false;
    }
    inline static bool to_native(::v8::Local<::v8::Value> v8_value, float& out_v)
    {
        auto isolate = ::v8::Isolate::GetCurrent();
        auto context = isolate->GetCurrentContext();

        if (v8_value->IsNumber())
        {
            out_v = (float)v8_value->NumberValue(context).ToChecked();
            return true;
        }
        return false;
    }
    inline static bool to_native(::v8::Local<::v8::Value> v8_value, double& out_v)
    {
        auto isolate = ::v8::Isolate::GetCurrent();
        auto context = isolate->GetCurrentContext();

        if (v8_value->IsNumber())
        {
            out_v = (double)v8_value->NumberValue(context).ToChecked();
            return true;
        }
        return false;
    }
    inline static bool to_native(::v8::Local<::v8::Value> v8_value, bool& out_v)
    {
        auto isolate = ::v8::Isolate::GetCurrent();

        if (v8_value->IsBoolean())
        {
            out_v = v8_value->BooleanValue(isolate);
            return true;
        }
        return false;
    }
    inline static bool to_native(::v8::Local<::v8::Value> v8_value, skr::String& out_v)
    {
        auto isolate = ::v8::Isolate::GetCurrent();

        if (v8_value->IsString())
        {
            out_v = skr::String::From(*::v8::String::Utf8Value(isolate, v8_value));
            return true;
        }
        return false;
    }

    // match helper
    static bool match_type(::v8::Local<::v8::Value> v8_value, rttr::TypeSignatureView native_signature);
    static bool match_params(const rttr::CtorData* data, const ::v8::FunctionCallbackInfo<::v8::Value>& info);
    static bool match_params(const rttr::MethodData* data, const ::v8::FunctionCallbackInfo<::v8::Value>& info);
    static bool match_params(const rttr::StaticMethodData* data, const ::v8::FunctionCallbackInfo<::v8::Value>& info);
    static bool match_params(const rttr::ExternMethodData* data, const ::v8::FunctionCallbackInfo<::v8::Value>& info);
    static bool match_params(const rttr::FunctionData* data, const ::v8::FunctionCallbackInfo<::v8::Value>& info);

    // convert helper
    inline static ::v8::Local<::v8::String> str_to_v8(StringView view, ::v8::Isolate* isolate, bool as_literal = false) {
        return ::v8::String::NewFromUtf8(
            isolate, 
            view.data_raw(), 
            as_literal ? ::v8::NewStringType::kInternalized : ::v8::NewStringType::kNormal, 
            view.length_buffer()
        ).ToLocalChecked();
    }
    static bool is_primitive(rttr::TypeSignatureView signature);
    static bool native_to_v8_primitive(
        ::v8::Local<::v8::Context> context,
        ::v8::Isolate*             isolate,
        rttr::TypeSignatureView    signature,
        void*                      native_data,
        ::v8::Local<::v8::Value>&  out_v8_value
    );
    static bool v8_to_native_primitive(
        ::v8::Local<::v8::Context> context,
        ::v8::Isolate*             isolate,
        rttr::TypeSignatureView    signature,
        ::v8::Local<::v8::Value>   v8_value,
        void*                      out_native_data,
        bool                       is_init
    );
    static bool native_to_v8_box(
        ::v8::Local<::v8::Context> context,
        ::v8::Isolate*             isolate,
        rttr::TypeSignatureView    signature,
        void*                      native_data,
        ::v8::Local<::v8::Value>&  out_v8_value
    );
    static bool v8_to_native_box(
        ::v8::Local<::v8::Context> context,
        ::v8::Isolate*             isolate,
        rttr::TypeSignatureView    signature,
        ::v8::Local<::v8::Value>   v8_value,
        void*                      out_native_data,
        bool                       is_init
    );
    //TODO. wrap bind use V8Isolate, wrap bind cannot be value signature
    
    // function invoke helpers
    static void push_param(
        rttr::DynamicStack&        stack,
        const rttr::ParamData&     param_data,
        ::v8::Local<::v8::Context> context,
        ::v8::Isolate*             isolate,
        ::v8::Local<::v8::Value>   v8_value
    );
    static bool read_return(
        rttr::DynamicStack&        stack, 
        rttr::TypeSignatureView    signature,
        ::v8::Local<::v8::Context> context,
        ::v8::Isolate*             isolate,
        ::v8::Local<::v8::Value>&  out_value
    );

    // caller
    static void call_ctor(
        void*                                          obj,
        const rttr::CtorData&                          data,
        const ::v8::FunctionCallbackInfo<::v8::Value>& info,
        ::v8::Local<::v8::Context>                     context,
        ::v8::Isolate*                                 isolate
    );
    static void call_method(
        void*                                          obj,
        const Vector<rttr::ParamData>&                 params,
        const rttr::TypeSignatureView                  ret_type,
        rttr::MethodInvokerDynamicStack                invoker,
        const ::v8::FunctionCallbackInfo<::v8::Value>& info,
        ::v8::Local<::v8::Context>                     context,
        ::v8::Isolate*                                 isolate
    );
    static void call_function(
        const Vector<rttr::ParamData>&                 params,
        const rttr::TypeSignatureView                  ret_type,
        rttr::FuncInvokerDynamicStack                  invoker,
        const ::v8::FunctionCallbackInfo<::v8::Value>& info,
        ::v8::Local<::v8::Context>                     context,
        ::v8::Isolate*                                 isolate
    );
};
} // namespace skr