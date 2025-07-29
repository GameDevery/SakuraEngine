#include <SkrV8/bind_template/v8bt_record_base.hpp>
#include <SkrV8/v8_bind.hpp>
#include <SkrV8/v8_bind_proxy.hpp>

// v8 includes
#include <libplatform/libplatform.h>
#include <v8-initialization.h>
#include <v8-template.h>
#include <v8-external.h>
#include <v8-function.h>
#include <v8-container.h>

namespace skr
{
void V8BTRecordBase::_setup(IV8BindManager* manager, const RTTRType* type)
{
    auto& _logger = manager->logger();

    // get basic data
    set_manager(manager);
    _rttr_type    = type;
    _default_ctor = type->find_default_ctor();
    if (!_default_ctor)
    {
        _logger.error(u8"record type '{}' has no default ctor", type->name());
    }
    _dtor = type->dtor_invoker();

    // export ctor
    _is_script_newable = flag_all(type->record_flag(), ERTTRRecordFlag::ScriptNewable);
    type->each_ctor([&](const RTTRCtorData* ctor) {
        type->each_ctor([&](const RTTRCtorData* ctor) {
            if (!flag_all(ctor->flag, ERTTRCtorFlag::ScriptVisible)) { return; }
            if (_ctor.is_valid())
            {
                _logger.error(u8"overload is not supported yet for ctor '{}'", type->name());
                return;
            }
        });
    });

    // export mixin
    Set<const RTTRMethodData*> mixin_consumed_methods;
    type->each_method([&](const RTTRMethodData* method, const RTTRType* owner_type) {
        if (!flag_all(owner_type->record_flag(), ERTTRRecordFlag::ScriptVisible)) { return; }
        if (!flag_all(method->flag, ERTTRMethodFlag::ScriptVisible)) { return; }
        if (!flag_all(method->flag, ERTTRMethodFlag::ScriptMixin)) { return; }

        mixin_consumed_methods.add(method);

        // find mixin impl
        String impl_method_name  = String::Build(method->name, u8"_impl");
        auto   found_impl_method = owner_type->find_method({
              .name          = impl_method_name,
              .include_bases = false,
        });
        if (!found_impl_method)
        {
            _logger.error(u8"miss impl for mixin method '{}', witch should be named '{}'", method->name, impl_method_name);
            return;
        }
        mixin_consumed_methods.add(found_impl_method);

        // export
        if (auto found_mixin = _methods.find(method->name))
        {
            _logger.error(u8"overload is not supported yet for mixin method '{}'", method->name);
        }
        else
        {
            auto& mixin_method_data                = _methods.try_add_default(method->name, found_mixin).value();
            mixin_method_data.rttr_data_mixin_impl = found_impl_method;
            mixin_method_data.setup(
                manager,
                method,
                owner_type
            );
        }
        // clang-format off
    }, {.include_bases = true});
    // clang-format on

    // export fields
    type->each_field([&](const RTTRFieldData* field, const RTTRType* owner_type) {
        if (!flag_all(owner_type->record_flag(), ERTTRRecordFlag::ScriptVisible)) { return; }
        if (!flag_all(field->flag, ERTTRFieldFlag::ScriptVisible)) { return; }

        auto& field_data = _fields.try_add_default(field->name).value();
        field_data.setup(
            manager,
            field,
            owner_type
        );
        // clang-format off
    }, { .include_bases = true });
    // clang-format on

    // export static field
    type->each_static_field([&](const RTTRStaticFieldData* static_field, const RTTRType* owner_type) {
        if (!flag_all(owner_type->record_flag(), ERTTRRecordFlag::ScriptVisible)) { return; }
        if (!flag_all(static_field->flag, ERTTRStaticFieldFlag::ScriptVisible)) { return; }

        auto& static_field_data = _static_fields.try_add_default(static_field->name).value();
        static_field_data.setup(
            manager,
            static_field,
            owner_type
        );
        // clang-format off
    }, { .include_bases = true });
    // clang-format on

    // export methods & properties
    type->each_method([&](const RTTRMethodData* method, const RTTRType* owner_type) {
        if (!flag_all(owner_type->record_flag(), ERTTRRecordFlag::ScriptVisible)) { return; }
        if (!flag_all(method->flag, ERTTRMethodFlag::ScriptVisible)) { return; }
        if (mixin_consumed_methods.contains(method)) { return; }

        auto find_getter_result = method->attrs.find_if([&](const Any& attr) {
            return attr.type_is<skr::attr::ScriptGetter>();
        });
        auto find_setter_result = method->attrs.find_if([&](const Any& attr) {
            return attr.type_is<skr::attr::ScriptSetter>();
        });

        // check flag error
        if (find_getter_result && find_setter_result)
        {
            _logger.error(u8"getter and setter applied on same method {}", method->name);
            return;
        }

        if (find_getter_result)
        { // getter case
            String prop_name  = find_getter_result.ref().cast<skr::attr::ScriptGetter>()->prop_name;
            auto&  prop_data  = _properties.try_add_default(prop_name).value();
            auto   _log_stack = _logger.stack(u8"export property '{}' getter", prop_name);
            if (prop_data.getter.is_valid())
            {
                _logger.error(u8"overload is not supported yet for property '{}'", prop_name);
            }
            else
            {
                prop_data.getter.setup(
                    manager,
                    method,
                    owner_type
                );
            }
        }
        else if (find_setter_result)
        { // setter case
            String prop_name  = find_setter_result.ref().cast<skr::attr::ScriptSetter>()->prop_name;
            auto&  prop_data  = _properties.try_add_default(prop_name).value();
            auto   _log_stack = _logger.stack(u8"export property '{}' setter", prop_name);
            if (prop_data.setter.is_valid())
            {
                _logger.error(u8"overload is not supported yet for property '{}'", prop_name);
            }
            else
            {
                prop_data.setter.setup(
                    manager,
                    method,
                    owner_type
                );
            }
        }
        else
        { // normal method
            if (auto found_method = _methods.find(method->name))
            {
                _logger.error(u8"overload is not supported yet for method '{}'", method->name);
            }
            else
            {
                // add method
                auto& method_data = _methods.try_add_default(method->name, found_method).value();
                method_data.setup(
                    manager,
                    method,
                    owner_type
                );
            }
        }
        // clang-format off
    }, { .include_bases = true });
    // clang-format on

    // export static methods & properties
    type->each_static_method([&](const RTTRStaticMethodData* method, const RTTRType* owner_type) {
        if (!flag_all(owner_type->record_flag(), ERTTRRecordFlag::ScriptVisible)) { return; }
        if (!flag_all(method->flag, ERTTRStaticMethodFlag::ScriptVisible)) { return; }

        auto find_getter_result = method->attrs.find_if([&](const Any& attr) {
            return attr.type_is<skr::attr::ScriptGetter>();
        });
        auto find_setter_result = method->attrs.find_if([&](const Any& attr) {
            return attr.type_is<skr::attr::ScriptSetter>();
        });

        // check flag error
        if (find_getter_result && find_setter_result)
        {
            _logger.error(u8"getter and setter applied on same static method{}", method->name);
            return;
        }

        if (find_getter_result)
        { // getter case
            String prop_name  = find_getter_result.ref().cast<skr::attr::ScriptGetter>()->prop_name;
            auto&  prop_data  = _static_properties.try_add_default(prop_name).value();
            auto   _log_stack = _logger.stack(u8"export static getter '{}'", prop_name);
            if (prop_data.getter.is_valid())
            {
                _logger.error(u8"overload is not supported yet for static property '{}'", prop_name);
            }
            else
            {
                prop_data.getter.setup(
                    manager,
                    method,
                    owner_type
                );
            }
        }
        else if (find_setter_result)
        { // setter case
            String prop_name  = find_setter_result.ref().cast<skr::attr::ScriptSetter>()->prop_name;
            auto&  prop_data  = _static_properties.try_add_default(prop_name).value();
            auto   _log_stack = _logger.stack(u8"export static setter '{}'", prop_name);
            if (prop_data.setter.is_valid())
            {
                _logger.error(u8"overload is not supported yet for static property '{}'", prop_name);
            }
            else
            {
                prop_data.setter.setup(
                    manager,
                    method,
                    owner_type
                );
            }
        }
        else
        { // normal method
            if (auto found_method = _static_methods.find(method->name))
            {
                _logger.error(u8"overload is not supported yet for static method '{}'", method->name);
            }
            else
            {
                // add method
                auto& method_data = _static_methods.try_add_default(method->name, found_method).value();
                method_data.setup(
                    manager,
                    method,
                    owner_type
                );
            }
        }
        // clang-format off
    }, { .include_bases = true });
    // clang-format on

    // TODO. check properties conflict
}

void V8BTRecordBase::_call_method(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    Isolate*    Isolate = info.GetIsolate();
    HandleScope HandleScope(Isolate);

    // block ctor call
    if (info.IsConstructCall())
    {
        Isolate->ThrowError("method can not be called with new");
        return;
    }

    // get external data
    auto* bind_proxy = get_bind_proxy<V8BPRecord>(info);
    auto* bind_data  = get_bind_data<V8BTDataMethod>(info);

    // check bind proxy
    if (!bind_proxy->is_valid())
    {
        Isolate->ThrowError("calling on a released object");
        return;
    }

    // match params
    if (!bind_data->match_param(info))
    {
        Isolate->ThrowError("params mismatch");
        return;
    }

    // invoke method
    bind_data->call(
        info,
        bind_proxy->address,
        bind_proxy->rttr_type
    );
}
void V8BTRecordBase::_call_static_method(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    Isolate*    Isolate = info.GetIsolate();
    HandleScope HandleScope(Isolate);

    // block ctor call
    if (info.IsConstructCall())
    {
        Isolate->ThrowError("method can not be called with new");
        return;
    }

    // get external data
    auto* bind_data = get_bind_data<V8BTDataStaticMethod>(info);

    // match params
    if (!bind_data->match(info))
    {
        Isolate->ThrowError("params mismatch");
        return;
    }

    // invoke method
    bind_data->call(
        info
    );
}
void V8BTRecordBase::_get_field(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    Isolate*    Isolate = info.GetIsolate();
    HandleScope HandleScope(Isolate);

    // get external data
    auto* bind_proxy = get_bind_proxy<V8BPRecord>(info);
    auto* bind_data  = get_bind_data<V8BTDataField>(info);

    // check bind proxy
    if (!bind_proxy->is_valid())
    {
        Isolate->ThrowError("calling on a released object");
        return;
    }

    // get field
    auto v8_field = bind_data->bind_tp->get_field(
        bind_proxy->address,
        bind_proxy->rttr_type,
        *bind_data
    );
    info.GetReturnValue().Set(v8_field);
}
void V8BTRecordBase::_set_field(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    Isolate*    Isolate = info.GetIsolate();
    HandleScope HandleScope(Isolate);

    // get external data
    auto* bind_proxy = get_bind_proxy<V8BPRecord>(info);
    auto* bind_data  = get_bind_data<V8BTDataField>(info);

    // check bind proxy
    if (!bind_proxy->is_valid())
    {
        Isolate->ThrowError("calling on a released object");
        return;
    }

    // set field
    bind_data->bind_tp->set_field(
        info[0],
        bind_proxy->address,
        bind_proxy->rttr_type,
        *bind_data
    );
}
void V8BTRecordBase::_get_static_field(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    Isolate*    Isolate = info.GetIsolate();
    HandleScope HandleScope(Isolate);

    // get external data
    auto* bind_data = get_bind_data<V8BTDataStaticField>(info);

    // get field
    auto v8_field = bind_data->bind_tp->get_static_field(
        *bind_data
    );
    info.GetReturnValue().Set(v8_field);
}
void V8BTRecordBase::_set_static_field(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    Isolate*    Isolate = info.GetIsolate();
    HandleScope HandleScope(Isolate);

    // get external data
    auto* bind_data = get_bind_data<V8BTDataStaticField>(info);

    // set field
    bind_data->bind_tp->set_static_field(
        info[0],
        *bind_data
    );
}
void V8BTRecordBase::_get_prop(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    Isolate*    Isolate = info.GetIsolate();
    HandleScope HandleScope(Isolate);

    // get external data
    auto* bind_proxy = get_bind_proxy<V8BPRecord>(info);
    auto* bind_data  = get_bind_data<V8BTDataProperty>(info);

    // check bind proxy
    if (!bind_proxy->is_valid())
    {
        Isolate->ThrowError("calling on a released object");
        return;
    }

    // match property
    if (!bind_data->getter.match_param(info))
    {
        Isolate->ThrowError("property getter params mismatch");
        return;
    }

    // invoke getter
    bind_data->getter.call(
        info,
        bind_proxy->address,
        bind_proxy->rttr_type
    );
}
void V8BTRecordBase::_set_prop(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    Isolate*    Isolate = info.GetIsolate();
    HandleScope HandleScope(Isolate);

    // get external data
    auto* bind_proxy = get_bind_proxy<V8BPRecord>(info);
    auto* bind_data  = get_bind_data<V8BTDataProperty>(info);

    // check bind proxy
    if (!bind_proxy->is_valid())
    {
        Isolate->ThrowError("calling on a released object");
        return;
    }

    // match
    if (!bind_data->setter.match_param(info))
    {
        Isolate->ThrowError("property setter params mismatch");
        return;
    }

    // invoke setter
    bind_data->setter.call(
        info,
        bind_proxy->address,
        bind_proxy->rttr_type
    );
}
void V8BTRecordBase::_get_static_prop(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    Isolate*    Isolate = info.GetIsolate();
    HandleScope HandleScope(Isolate);

    // get external data
    auto* bind_data = get_bind_data<V8BTDataStaticProperty>(info);

    // match
    if (!bind_data->getter.match(info))
    {
        Isolate->ThrowError("property getter params mismatch");
        return;
    }

    // invoke getter
    bind_data->getter.call(
        info
    );
}
void V8BTRecordBase::_set_static_prop(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    Isolate*    Isolate = info.GetIsolate();
    HandleScope HandleScope(Isolate);

    // get external data
    auto* bind_data = get_bind_data<V8BTDataStaticProperty>(info);

    // match
    if (!bind_data->setter.match(info))
    {
        Isolate->ThrowError("property setter params mismatch");
        return;
    }

    // invoke setter
    bind_data->setter.call(
        info
    );
}
} // namespace skr