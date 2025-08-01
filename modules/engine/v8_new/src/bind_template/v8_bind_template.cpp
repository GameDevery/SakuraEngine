#include <SkrV8/bind_template/v8_bind_template.hpp>
#include <SkrV8/v8_bind.hpp>
#include <SkrV8/v8_bind_proxy.hpp>

// v8 includes
#include <libplatform/libplatform.h>
#include <v8-initialization.h>
#include <v8-template.h>
#include <v8-external.h>
#include <v8-function.h>
#include <v8-container.h>

// helper
namespace skr::helper
{
inline static bool match_params(
    span<const V8BTDataParam>                      params,
    uint32_t                                       solved_param_count,
    const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack
)
{
    // check param count
    if (solved_param_count != v8_stack.Length()) { return {}; }

    // check param type
    for (size_t i = 0; i < params.size(); i++)
    {
        auto& param_data = params[i];
        auto  v8_arg     = v8_stack[i];

        // skip pure out param
        if (param_data.inout_flag == ERTTRParamFlag::Out) { continue; }

        // do match
        bool matched = param_data.bind_tp->match_param(v8_arg);
        if (!matched) { return false; }
    }

    return true;
}
inline static v8::Local<v8::Value> read_return(
    DynamicStack&             stack,
    span<const V8BTDataParam> params,
    const V8BTDataReturn&     return_bind_tp,
    uint32_t                  solved_return_count
)
{
    auto* isolate = v8::Isolate::GetCurrent();
    auto  context = isolate->GetCurrentContext();

    if (solved_return_count == 1)
    { // return single value
        if (return_bind_tp.is_void)
        { // read from out param
            for (const auto& param_bind_tp : params)
            {
                if (param_bind_tp.appare_in_return)
                {
                    return param_bind_tp.bind_tp->read_return_from_out_param(
                        stack,
                        param_bind_tp
                    );
                }
            }
        }
        else
        { // read return value
            if (stack.is_return_stored())
            {
                return return_bind_tp.bind_tp->read_return_native(
                    stack,
                    return_bind_tp
                );
            }
        }
    }
    else
    { // return param array
        v8::Local<v8::Array> out_array = v8::Array::New(isolate);
        uint32_t             cur_index = 0;

        // try read return value
        if (!return_bind_tp.is_void)
        {
            if (stack.is_return_stored())
            {
                v8::Local<v8::Value> out_value = return_bind_tp.bind_tp->read_return_native(
                    stack,
                    return_bind_tp
                );
                out_array->Set(context, cur_index, out_value).Check();
                ++cur_index;
            }
        }

        // read return value from out param
        for (const auto& param_bind_tp : params)
        {
            if (param_bind_tp.appare_in_return)
            {
                // clang-format off
                out_array->Set(
                    context, 
                    cur_index, 
                param_bind_tp.bind_tp->read_return_from_out_param(
                    stack, 
                    param_bind_tp
                )).Check();
                // clang-format on
                ++cur_index;
            }
        }

        return out_array;
    }

    return {};
}
} // namespace skr::helper

namespace skr
{
//===============================V8BTDataField===============================
void V8BTDataField::setup(
    IV8BindManager*      manager,
    const RTTRFieldData* field_data,
    const RTTRType*      owner
)
{
    {
        auto type_sig_view = field_data->type.view();
        type_sig_view.jump_modifier();
        bind_tp = manager->solve_bind_tp(type_sig_view);
    }
    field_owner = owner;
    rttr_data   = field_data;
    modifiers.solve(field_data->type.view());
    bind_tp->solve_field(*this);
}

//===============================V8BTDataStaticField===============================
void V8BTDataStaticField::setup(
    IV8BindManager*            manager,
    const RTTRStaticFieldData* field_data,
    const RTTRType*            owner
)
{
    {
        auto type_sig_view = field_data->type.view();
        type_sig_view.jump_modifier();
        bind_tp = manager->solve_bind_tp(type_sig_view);
    }
    field_owner = owner;
    rttr_data   = field_data;
    modifiers.solve(field_data->type.view());
    bind_tp->solve_static_field(*this);
}

//===============================V8BTDataParam===============================
void V8BTDataParam::setup(
    IV8BindManager*      manager,
    const RTTRParamData* param_data
)
{
    auto& _logger = manager->logger();

    {
        auto type_sig_view = param_data->type.view();
        type_sig_view.jump_modifier();
        bind_tp = manager->solve_bind_tp(type_sig_view);
    }
    rttr_data = param_data;
    index     = param_data->index;

    // solve modifier
    {
        auto type_sig_view = param_data->type.view();
        modifiers.solve(type_sig_view);
        bool has_param_out = flag_all(param_data->flag, ERTTRParamFlag::Out);
        bool has_param_in  = flag_all(param_data->flag, ERTTRParamFlag::In);

        // check pointer level
        if (type_sig_view.decayed_pointer_level() > 1)
        {
            _logger.error(u8"pointer level greater than 1");
            return;
        }

        // solve inout flag
        if (!has_param_in && !has_param_out)
        { // use param default inout flag
            if (modifiers.is_any_ref() && !modifiers.is_const)
            {
                inout_flag |= ERTTRParamFlag::In | ERTTRParamFlag::Out;
            }
        }
        else
        {
            if (has_param_in)
            {
                inout_flag |= ERTTRParamFlag::In;
            }
            if (has_param_out)
            {
                inout_flag |= ERTTRParamFlag::Out;
                if (!modifiers.is_any_ref() || modifiers.is_const)
                {
                    _logger.error(u8"only T& can be out param");
                }
            }
        }
    }
}

//===============================V8BTDataParam===============================
void V8BTDataReturn::setup(
    IV8BindManager*   manager,
    TypeSignatureView signature
)
{
    auto& _logger    = manager->logger();
    auto  _log_stack = _logger.stack(u8"export return");

    {
        auto type_sig_view = signature;
        modifiers.solve(type_sig_view);
        type_sig_view.jump_modifier();
        bind_tp = manager->solve_bind_tp(type_sig_view);
    }
    pass_by_ref = signature.is_decayed_pointer();

    bind_tp->solve_return(*this);
}

//===============================V8BTDataMethod===============================
bool V8BTDataMethod::match_param(
    const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack
) const
{
    return helper::match_params(
        params_data,
        params_count,
        v8_stack
    );
}
void V8BTDataMethod::call(
    const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack,
    void*                                          obj,
    const RTTRType*                                obj_type
) const
{
    DynamicStack native_stack;

    // push param
    uint32_t v8_stack_index = 0;
    for (const auto& param : params_data)
    {
        if (param.inout_flag == ERTTRParamFlag::Out)
        { // pure out param, we will push a dummy xvalue
            param.bind_tp->push_param_native_pure_out(
                native_stack,
                param
            );
        }
        else
        {
            param.bind_tp->push_param_native(
                native_stack,
                param,
                v8_stack[v8_stack_index]
            );
            ++v8_stack_index;
        }
    }

    // get call obj address
    void* owner_address = obj_type->cast_to_base(obj_type->type_id(), obj);

    // invoke
    native_stack.return_behaviour_store();
    if (rttr_data_mixin_impl)
    { // mixin case, invoke minin impl
        rttr_data_mixin_impl->dynamic_stack_invoke(owner_address, native_stack);
    }
    else
    { // normal case
        rttr_data->dynamic_stack_invoke(owner_address, native_stack);
    }

    // read return
    auto return_value = helper::read_return(
        native_stack,
        params_data,
        return_data,
        return_count
    );
    if (!return_value.IsEmpty())
    {
        v8_stack.GetReturnValue().Set(return_value);
    }
}
void V8BTDataMethod::setup(
    IV8BindManager*       manager,
    const RTTRMethodData* method_data,
    const RTTRType*       owner
)
{
    method_owner = owner;
    rttr_data    = method_data;

    // setup info
    return_data.setup(manager, method_data->ret_type);
    for (const auto* param : method_data->param_data)
    {
        auto& param_binder = params_data.add_default().ref();
        param_binder.setup(manager, param);
    }

    // solve count
    return_count = return_data.is_void ? 0 : 1;
    params_count = 0;
    for (const auto& param : params_data)
    {
        if (param.appare_in_param) ++params_count;
        if (param.appare_in_return) ++return_count;
    }
}

//===============================V8BTDataStaticMethod===============================
bool V8BTDataStaticMethod::match(
    const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack
) const
{
    return helper::match_params(
        params_data,
        params_count,
        v8_stack
    );
}
void V8BTDataStaticMethod::call(
    const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack
) const
{
    DynamicStack native_stack;

    // push param
    uint32_t v8_stack_index = 0;
    for (const auto& param : params_data)
    {
        if (param.inout_flag == ERTTRParamFlag::Out)
        { // pure out param, we will push a dummy xvalue
            param.bind_tp->push_param_native_pure_out(
                native_stack,
                param
            );
        }
        else
        {
            param.bind_tp->push_param_native(
                native_stack,
                param,
                v8_stack[v8_stack_index]
            );
            ++v8_stack_index;
        }
    }

    // invoke
    native_stack.return_behaviour_store();
    rttr_data->dynamic_stack_invoke(native_stack);

    // read return
    auto return_value = helper::read_return(
        native_stack,
        params_data,
        return_data,
        return_count
    );
    if (!return_value.IsEmpty())
    {
        v8_stack.GetReturnValue().Set(return_value);
    }
}
void V8BTDataStaticMethod::setup(
    IV8BindManager*             manager,
    const RTTRStaticMethodData* method_data,
    const RTTRType*             owner
)
{
    method_owner = owner;
    rttr_data    = method_data;
    return_data.setup(manager, method_data->ret_type);
    for (const auto* param : method_data->param_data)
    {
        auto& param_binder = params_data.add_default().ref();
        param_binder.setup(manager, param);
    }

    // solve count
    return_count = return_data.is_void ? 0 : 1;
    params_count = 0;
    for (const auto& param : params_data)
    {
        if (param.appare_in_param) ++params_count;
        if (param.appare_in_return) ++return_count;
    }
}

//===============================V8BTDataCtor===============================
bool V8BTDataCtor::match(
    const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack
) const
{
    return helper::match_params(
        params_data,
        params_data.size(),
        v8_stack
    );
}
void V8BTDataCtor::call(
    const ::v8::FunctionCallbackInfo<::v8::Value>& v8_stack,
    void*                                          obj
) const
{
    DynamicStack native_stack;

    // push param
    uint32_t v8_stack_index = 0;
    for (const auto& param : params_data)
    {
        param.bind_tp->push_param_native(
            native_stack,
            param,
            v8_stack[v8_stack_index]
        );
        ++v8_stack_index;
    }

    // invoke
    native_stack.return_behaviour_discard();
    rttr_data->dynamic_stack_invoke(obj, native_stack);
}
void V8BTDataCtor::setup(
    IV8BindManager*     manager,
    const RTTRCtorData* ctor_data
)
{
    auto& _logger    = manager->logger();
    auto  _log_stack = _logger.stack(u8"export ctor");

    rttr_data = ctor_data;

    // export params
    for (const auto* param : ctor_data->param_data)
    {
        auto& param_binder = params_data.add_default().ref();
        param_binder.setup(manager, param);
    }

    // check out flag
    for (const auto& param_binder : params_data)
    {
        if (flag_all(param_binder.inout_flag, ERTTRParamFlag::Out))
        {
            auto _param_log_stack = _logger.stack(
                u8"export param '{}', index {}",
                param_binder.rttr_data->name,
                param_binder.rttr_data->index
            );
            _logger.error(u8"ctor param cannot has out flag");
        }
    }
}
} // namespace skr