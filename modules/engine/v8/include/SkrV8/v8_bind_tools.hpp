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

    // param & return helper
    inline static bool push_param(
        rttr::DynamicStack&        stack, 
        const rttr::ParamData&     param_data,
        ::v8::Local<::v8::Value>   v8_value,
        ::v8::Local<::v8::Context> context,
        ::v8::Isolate*             isolate
    )
    {
        auto native_signature = param_data.type.view();

        if (v8_value.IsEmpty() || v8_value->IsUndefined())
        {
            // TODO. default value
            SKR_UNIMPLEMENTED_FUNCTION()
        }
        else
        {
            if (native_signature.is_type())
            {
                // export primitive type
                auto pure_type_signature = native_signature;
                pure_type_signature.jump_modifier();
                GUID type_id;
                pure_type_signature.read_type_id(type_id);

                // export primitive
                if (is_primitive(pure_type_signature))
                {
                    // get type
                    auto* type = rttr::get_type_from_guid(type_id);

                    // push stack
                    void* data = stack.alloc_param_raw(
                        type->size(),
                        type->alignment(),
                        rttr::EDynamicStackParamKind::Direct,
                        nullptr
                    );

                    // write data
                    v8_to_native_primitive(
                        context,
                        isolate,
                        pure_type_signature,
                        v8_value,
                        data
                    );
                    return true;
                }
                else
                {
                    // TODO. export record type
                    SKR_UNIMPLEMENTED_FUNCTION()
                }
            }
            else
            {
                // TODO. export generic type
                SKR_UNIMPLEMENTED_FUNCTION()
            }
        }

        return false;
    }
    inline static bool read_return(
        rttr::DynamicStack&        stack, 
        rttr::TypeSignatureView    signature,
        ::v8::Local<::v8::Value>&  out_value,
        ::v8::Local<::v8::Context> context,
        ::v8::Isolate*             isolate
    )
    {
        if (signature.is_type())
        {
            if (native_to_v8_primitive(
                context,
                isolate,
                signature,
                stack.get_return_raw(),
                out_value
            ))
            {
                return true;
            }
            else
            {
                // TODO. handle record type
                SKR_UNIMPLEMENTED_FUNCTION()
            }
        }
        else 
        {
            // TODO. handle generic type
            SKR_UNIMPLEMENTED_FUNCTION()
        }
        return false;
    }
    

    // call helper
    inline static void call_ctor(
        void*                                          obj,
        const rttr::CtorData&                          data,
        const ::v8::FunctionCallbackInfo<::v8::Value>& info,
        ::v8::Local<::v8::Context>                     context,
        ::v8::Isolate*                                 isolate)
    {
        rttr::DynamicStack stack;

        // combine proxy
        for (size_t i = 0; i < data.param_data.size(); ++i)
        {
            push_param(
                stack,
                data.param_data[i],
                i < info.Length() ? info[i] : ::v8::Local<::v8::Value>{},
                context,
                isolate
            );
        }

        // invoke
        stack.return_behaviour_discard();
        data.dynamic_stack_invoke(obj, stack);
    }
    inline static void call_method(
        void*                                          obj,
        const rttr::MethodData&                        data,
        const ::v8::FunctionCallbackInfo<::v8::Value>& info,
        ::v8::Local<::v8::Context>                     context,
        ::v8::Isolate*                                 isolate)
    {
        rttr::DynamicStack stack;

        // combine proxy
        for (size_t i = 0; i < data.param_data.size(); ++i)
        {
            push_param(
                stack,
                data.param_data[i],
                i < info.Length() ? info[i] : ::v8::Local<::v8::Value>{},
                context,
                isolate
            );
        }

        // invoke
        stack.return_behaviour_store();
        data.dynamic_stack_invoke(obj, stack);

        // read return
        ::v8::Local<::v8::Value> out_value;
        if (read_return(
            stack,
            data.ret_type.view(),
            out_value,
            context,
            isolate
        ))
        {
            info.GetReturnValue().Set(out_value);
        }
    }
    inline static void call_static_method(
        const rttr::StaticMethodData&                  data,
        const ::v8::FunctionCallbackInfo<::v8::Value>& info,
        ::v8::Local<::v8::Context>                     context,
        ::v8::Isolate*                                 isolate)
    {
        rttr::DynamicStack stack;

        // push param
        for (size_t i = 0; i < data.param_data.size(); ++i)
        {
            push_param(
                stack,
                data.param_data[i],
                i < info.Length() ? info[i] : ::v8::Local<::v8::Value>{},
                context,
                isolate
            );
        }

        // invoke
        stack.return_behaviour_store();
        data.dynamic_stack_invoke(stack);

        // read return
        ::v8::Local<::v8::Value> out_value;
        if (read_return(
            stack,
            data.ret_type.view(),
            out_value,
            context,
            isolate
        ))
        {
            info.GetReturnValue().Set(out_value);
        }
    }
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
    auto native_length = data.param_data.size();

    // length > data, must be unmatched
    if (call_length > native_length)
    {
        return false;
    }

    // match default param
    for (size_t i = call_length; i < native_length; ++i)
    {
        if (!data.param_data[i].make_default)
        {
            return false;
        }
    }

    // match signature
    for (size_t i = 0; i < call_length; ++i)
    {
        Local<Value>            call_value       = info[i];
        rttr::TypeSignatureView native_signature = data.param_data[i].type.view();

        // use default value
        if (call_value->IsUndefined())
        {
            if (!data.param_data[i].make_default)
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