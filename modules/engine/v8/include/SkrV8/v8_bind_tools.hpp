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
    template <typename Data>
    static bool match_params(const Data& data, const ::v8::FunctionCallbackInfo<::v8::Value>& info);

    // convert helper
    static bool is_primitive(rttr::TypeSignatureView signature);
    static bool native_to_v8_primitive(
        ::v8::Local<::v8::Context> context,
        ::v8::Isolate*             isolate,
        rttr::TypeSignatureView    signature,
        void*                      native_data,
        ::v8::Local<::v8::Value>&  out_v8_value);
    static bool v8_to_native_primitive(
        ::v8::Local<::v8::Context> context,
        ::v8::Isolate*             isolate,
        rttr::TypeSignatureView    signature,
        ::v8::Local<::v8::Value>   v8_value,
        void*                      out_native_data);
    
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
template <typename Data>
inline bool V8BindTools::match_params(const Data& data, const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;
    auto call_length   = info.Length();
    auto native_length = data->param_data.size();

    // length > data, must be unmatched
    if (call_length > native_length)
    {
        return false;
    }

    // match default param
    for (size_t i = call_length; i < native_length; ++i)
    {
        if (!data->param_data[i].make_default)
        {
            return false;
        }
    }

    // match signature
    for (size_t i = 0; i < call_length; ++i)
    {
        Local<Value>            call_value       = info[i];
        rttr::TypeSignatureView native_signature = data->param_data[i].type.view();

        // use default value
        if (call_value->IsUndefined())
        {
            if (!data->param_data[i].make_default)
            {
                return false;
            }
        }

        // match type
        if (!match_type(call_value, native_signature))
        {
            return false;
        }
    }

    return true;
}
} // namespace skr::v8