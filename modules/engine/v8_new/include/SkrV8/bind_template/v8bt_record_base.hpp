#pragma once
#include <SkrV8/bind_template/v8_bind_template.hpp>

namespace skr
{
struct V8BTRecordBase : V8BindTemplate {

protected:
    void _setup(IV8BindManager* manager, const RTTRType* type);

    static void _call_method(const ::v8::FunctionCallbackInfo<::v8::Value>& info);
    static void _call_static_method(const ::v8::FunctionCallbackInfo<::v8::Value>& info);
    static void _get_field(const ::v8::FunctionCallbackInfo<::v8::Value>& info);
    static void _set_field(const ::v8::FunctionCallbackInfo<::v8::Value>& info);
    static void _get_static_field(const ::v8::FunctionCallbackInfo<::v8::Value>& info);
    static void _set_static_field(const ::v8::FunctionCallbackInfo<::v8::Value>& info);
    static void _get_prop(const ::v8::FunctionCallbackInfo<::v8::Value>& info);
    static void _set_prop(const ::v8::FunctionCallbackInfo<::v8::Value>& info);
    static void _get_static_prop(const ::v8::FunctionCallbackInfo<::v8::Value>& info);
    static void _set_static_prop(const ::v8::FunctionCallbackInfo<::v8::Value>& info);

protected:
    const RTTRType*        _rttr_type         = nullptr;
    bool                   _is_script_newable = false;
    RTTRInvokerDefaultCtor _default_ctor      = nullptr;
    DtorInvoker            _dtor              = nullptr;

    V8BTDataCtor                        _ctor              = {};
    Map<String, V8BTDataField>          _fields            = {};
    Map<String, V8BTDataStaticField>    _static_fields     = {};
    Map<String, V8BTDataMethod>         _methods           = {};
    Map<String, V8BTDataStaticMethod>   _static_methods    = {};
    Map<String, V8BTDataProperty>       _properties        = {};
    Map<String, V8BTDataStaticProperty> _static_properties = {};
};
} // namespace skr