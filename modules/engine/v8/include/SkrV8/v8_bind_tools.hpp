#pragma once
#include "SkrV8/v8_bind.hpp"

namespace skr
{
struct SKR_V8_API V8BindTools {
    // match helper
    static bool match_type(::v8::Local<::v8::Value> v8_value, TypeSignatureView native_signature);
    static bool match_params(const RTTRCtorData* data, const ::v8::FunctionCallbackInfo<::v8::Value>& info);
    static bool match_params(const RTTRMethodData* data, const ::v8::FunctionCallbackInfo<::v8::Value>& info);
    static bool match_params(const RTTRStaticMethodData* data, const ::v8::FunctionCallbackInfo<::v8::Value>& info);
    static bool match_params(const RTTRExternMethodData* data, const ::v8::FunctionCallbackInfo<::v8::Value>& info);
    static bool match_params(const RTTRFunctionData* data, const ::v8::FunctionCallbackInfo<::v8::Value>& info);

    // convert helper
    static bool is_primitive(TypeSignatureView signature);
    static bool native_to_v8_primitive(
        ::v8::Local<::v8::Context> context,
        ::v8::Isolate*             isolate,
        TypeSignatureView          signature,
        void*                      native_data,
        ::v8::Local<::v8::Value>&  out_v8_value
    );
    static bool v8_to_native_primitive(
        ::v8::Local<::v8::Context> context,
        ::v8::Isolate*             isolate,
        TypeSignatureView          signature,
        ::v8::Local<::v8::Value>   v8_value,
        void*                      out_native_data,
        bool                       is_init
    );
    static bool native_to_v8_box(
        ::v8::Local<::v8::Context> context,
        ::v8::Isolate*             isolate,
        TypeSignatureView          signature,
        void*                      native_data,
        ::v8::Local<::v8::Value>&  out_v8_value
    );
    static bool v8_to_native_box(
        ::v8::Local<::v8::Context> context,
        ::v8::Isolate*             isolate,
        TypeSignatureView          signature,
        ::v8::Local<::v8::Value>   v8_value,
        void*                      out_native_data,
        bool                       is_init
    );
    // TODO. wrap bind use V8Isolate, wrap bind cannot be value signature

    // function invoke helpers
    static void push_param(
        DynamicStack&              stack,
        const RTTRParamData&       param_data,
        ::v8::Local<::v8::Context> context,
        ::v8::Isolate*             isolate,
        ::v8::Local<::v8::Value>   v8_value
    );
    static bool read_return(
        DynamicStack&              stack,
        TypeSignatureView          signature,
        ::v8::Local<::v8::Context> context,
        ::v8::Isolate*             isolate,
        ::v8::Local<::v8::Value>&  out_value
    );

    // caller
    static void call_ctor(
        void*                                          obj,
        const RTTRCtorData&                            data,
        const ::v8::FunctionCallbackInfo<::v8::Value>& info,
        ::v8::Local<::v8::Context>                     context,
        ::v8::Isolate*                                 isolate
    );
    static void call_method(
        void*                                          obj,
        const Vector<RTTRParamData*>&                  params,
        const TypeSignatureView                        ret_type,
        MethodInvokerDynamicStack                      invoker,
        const ::v8::FunctionCallbackInfo<::v8::Value>& info,
        ::v8::Local<::v8::Context>                     context,
        ::v8::Isolate*                                 isolate
    );
    static void call_function(
        const Vector<RTTRParamData*>&                  params,
        const TypeSignatureView                        ret_type,
        FuncInvokerDynamicStack                        invoker,
        const ::v8::FunctionCallbackInfo<::v8::Value>& info,
        ::v8::Local<::v8::Context>                     context,
        ::v8::Isolate*                                 isolate
    );
};
} // namespace skr