#pragma once
#include "v8-template.h"
#include "SkrRTTR/type_signature.hpp"
#include "SkrRTTR/type.hpp"
#include "SkrRTTR/export/export_data.hpp"

namespace skr::v8
{
struct SKR_V8_API V8BindTools {
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
} // namespace skr::v8

// inline impl
namespace skr::v8
{

} // namespace skr::v8