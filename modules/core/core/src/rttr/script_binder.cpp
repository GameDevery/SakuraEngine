#include "SkrCore/log.hpp"
#include "SkrRTTR/scriptble_object.hpp"
#include <SkrRTTR/script_binder.hpp>

namespace skr
{
// ctor & dtor
ScriptBinderManager::ScriptBinderManager()
{
}
ScriptBinderManager::~ScriptBinderManager()
{
    // delete root binder cache
    for (auto& [type_id, binder] : _cached_root_binders)
    {
        switch (binder.kind())
        {
        case ScriptBinderRoot::EKind::Primitive:
            SkrDelete(binder.primitive());
            break;
        case ScriptBinderRoot::EKind::Box:
            SkrDelete(binder.box());
            break;
        case ScriptBinderRoot::EKind::Wrap:
            SkrDelete(binder.wrap());
            break;
        }
    }
}

// get binder
ScriptBinderRoot ScriptBinderManager::get_or_build(GUID type_id)
{
    // find cache
    if (auto result = _cached_root_binders.find(type_id))
    {
        return result.value();
    }

    // make new binder
    if (auto* primitive_binder = _make_primitive(type_id))
    {
        _cached_root_binders.add(type_id, primitive_binder);
        return primitive_binder;
    }

    // box or wrap
    auto* type = get_type_from_guid(type_id);
    if (type)
    {
        // make box binder
        if (auto* box_binder = _make_box(type))
        {
            _cached_root_binders.add(type_id, box_binder);
            return box_binder;
        }

        // make wrap binder
        if (auto* wrap_binder = _make_wrap(type))
        {
            _cached_root_binders.add(type_id, wrap_binder);
            return wrap_binder;
        }
    }

    // failed to export
    _cached_root_binders.add(type_id, {});
    return {};
}

// make root binder
template <typename T>
ScriptBinderPrimitive* _new_primitive()
{
    ScriptBinderPrimitive* result = SkrNew<ScriptBinderPrimitive>();

    // setup primitive data
    result->size      = sizeof(T);
    result->alignment = alignof(T);
    result->type_id   = type_id_of<T>();
    if constexpr (std::is_trivially_destructible_v<T>)
    {
        result->dtor = nullptr;
    }
    else
    {
        result->dtor = +[](void* p) -> void {
            reinterpret_cast<T*>(p)->~T();
        };
    }

    return result;
}
ScriptBinderPrimitive* _new_void()
{
    ScriptBinderPrimitive* result = SkrNew<ScriptBinderPrimitive>();

    result->size      = 0;
    result->alignment = 0;
    result->type_id   = type_id_of<void>();
    result->dtor      = nullptr;

    return result;
}
ScriptBinderPrimitive* ScriptBinderManager::_make_primitive(GUID type_id)
{
    switch (type_id.get_hash())
    {
    case type_id_of<void>().get_hash():
        return _new_void();
    case type_id_of<int8_t>().get_hash():
        return _new_primitive<int8_t>();
    case type_id_of<int16_t>().get_hash():
        return _new_primitive<int16_t>();
    case type_id_of<int32_t>().get_hash():
        return _new_primitive<int32_t>();
    case type_id_of<int64_t>().get_hash():
        return _new_primitive<int64_t>();
    case type_id_of<uint8_t>().get_hash():
        return _new_primitive<uint8_t>();
    case type_id_of<uint16_t>().get_hash():
        return _new_primitive<uint16_t>();
    case type_id_of<uint32_t>().get_hash():
        return _new_primitive<uint32_t>();
    case type_id_of<uint64_t>().get_hash():
        return _new_primitive<uint64_t>();
    case type_id_of<float>().get_hash():
        return _new_primitive<float>();
    case type_id_of<double>().get_hash():
        return _new_primitive<double>();
    case type_id_of<bool>().get_hash():
        return _new_primitive<bool>();
    case type_id_of<skr::String>().get_hash():
        return _new_primitive<skr::String>();
    default:
        return nullptr;
    }
}
ScriptBinderBox* ScriptBinderManager::_make_box(const RTTRType* type)
{
    // clang-format off
    if (!flag_all(
        type->record_flag(),
        ERTTRRecordFlag::ScriptVisible | ERTTRRecordFlag::ScriptBox
    ))
    {
        return nullptr;
    }
    // clang-format on

    ScriptBinderBox result{};
    result.type = type;

    // each field
    type->each_field([&](const RTTRFieldData* field, const RTTRType* owner_type) {
        if (!flag_all(field->flag, ERTTRFieldFlag::ScriptVisible)) { return; }
        auto& field_data = result.fields.try_add_default(field->name).value();
        _make_field(field_data, field, owner_type);
    });

    // return result
    return SkrNew<ScriptBinderBox>(std::move(result));
}
ScriptBinderWrap* ScriptBinderManager::_make_wrap(const RTTRType* type)
{
    // clang-format off
    if (!flag_all(
        type->record_flag(),
        ERTTRRecordFlag::ScriptVisible
    ))
    {
        return nullptr;
    }
    // clang-format on

    // check base
    if (!type->based_on(type_id_of<ScriptbleObject>()))
    {
        SKR_LOG_FMT_ERROR(
            u8"failed to export {} as wrap type, must be based on ::skr::ScriptbleObject",
            type->name()
        );
        return nullptr;
    }

    ScriptBinderWrap result = {};
    result.type             = type;

    // export ctors
    result.is_script_newable = flag_all(type->record_flag(), ERTTRRecordFlag::ScriptNewable);
    if (result.is_script_newable)
    {
        type->each_ctor([&](const RTTRCtorData* ctor) {
            if (!flag_all(ctor->flag, ERTTRCtorFlag::ScriptVisible)) { return; }
            auto& ctor_data = result.ctors.add_default().ref();
            _make_ctor(ctor_data, ctor, type);
        });
    }

    // export fields
    type->each_field([&](const RTTRFieldData* field, const RTTRType* owner_type) {
        if (!flag_all(field->flag, ERTTRFieldFlag::ScriptVisible)) { return; }
        auto& field_data = result.fields.try_add_default(field->name).value();
        _make_field(field_data, field, owner_type);
    });

    // export static field
    type->each_static_field([&](const RTTRStaticFieldData* static_field, const RTTRType* owner_type) {
        if (!flag_all(static_field->flag, ERTTRStaticFieldFlag::ScriptVisible)) { return; }
        auto& static_field_data = result.static_fields.try_add_default(static_field->name).value();
        _make_static_field(static_field_data, static_field, owner_type);
    });

    // export methods
    type->each_method([&](const RTTRMethodData* method, const RTTRType* owner_type) {
        if (!flag_all(method->flag, ERTTRMethodFlag::ScriptVisible)) { return; }

        auto find_getter_result = method->attrs.find_if([&](const Any& attr) {
            return attr.type_is<skr::attr::ScriptGetter>();
        });
        auto find_setter_result = method->attrs.find_if([&](const Any& attr) {
            return attr.type_is<skr::attr::ScriptSetter>();
        });

        if (find_getter_result)
        {
            String prop_name = find_getter_result.ref().cast<skr::attr::ScriptGetter>()->prop_name;
            auto&  prop_data = result.properties.try_add_default(prop_name).value();
            _make_prop_getter(prop_data.getter, method, owner_type);
        }
        else if (find_setter_result)
        {
            String prop_name = find_setter_result.ref().cast<skr::attr::ScriptSetter>()->prop_name;
            auto&  prop_data = result.properties.try_add_default(prop_name).value();
            _make_prop_setter(prop_data.setter, method, owner_type);
        }
        else
        {
            auto& method_data   = result.methods.try_add_default(method->name).value();
            auto& overload_data = method_data.overloads.add_default().ref();
            _make_method(overload_data, method, owner_type);
        }
    });

    // export static methods
    type->each_static_method([&](const RTTRStaticMethodData* method, const RTTRType* owner_type) {
        if (!flag_all(method->flag, ERTTRStaticMethodFlag::ScriptVisible)) { return; }
        auto find_getter_result = method->attrs.find_if([&](const Any& attr) {
            return attr.type_is<skr::attr::ScriptGetter>();
        });
        auto find_setter_result = method->attrs.find_if([&](const Any& attr) {
            return attr.type_is<skr::attr::ScriptSetter>();
        });

        if (find_getter_result)
        {
            String prop_name = find_getter_result.ref().cast<skr::attr::ScriptGetter>()->prop_name;
            auto&  prop_data = result.static_properties.try_add_default(prop_name).value();
            _make_static_prop_getter(prop_data.getter, method, owner_type);
        }
        else if (find_setter_result)
        {
            String prop_name = find_setter_result.ref().cast<skr::attr::ScriptSetter>()->prop_name;
            auto&  prop_data = result.static_properties.try_add_default(prop_name).value();
            _make_static_prop_setter(prop_data.setter, method, owner_type);
        }
        else
        {
            auto& static_method_data = result.static_methods.try_add_default(method->name).value();
            auto& overload_data      = static_method_data.overloads.add_default().ref();
            _make_static_method(overload_data, method, owner_type);
        }
    });

    // return result
    return SkrNew<ScriptBinderWrap>(std::move(result));
}

// make nested binder
void ScriptBinderManager::_make_ctor(ScriptBinderCtor& out, const RTTRCtorData* ctor, const RTTRType* owner)
{
    auto _log_stack = _logger.stack(u8"export ctor");

    // basic data
    out.data = ctor;

    // export params
    for (const auto* param : ctor->param_data)
    {
        auto& param_binder = out.params_binder.add_default().ref();
        _make_param(param_binder, param);
    }
}
void ScriptBinderManager::_make_method(ScriptBinderMethod::Overload& out, const RTTRMethodData* method, const RTTRType* owner)
{
    auto _log_stack = _logger.stack(u8"export method '{}'", method->name);

    // basic data
    out.data  = method;
    out.owner = owner;

    // export params
    for (const auto* param : method->param_data)
    {
        auto& param_binder = out.params_binder.add_default().ref();
        _make_param(param_binder, param);
    }

    // export return
    _make_return(out.return_binder, method->ret_type.view());
}
void ScriptBinderManager::_make_static_method(ScriptBinderStaticMethod::Overload& out, const RTTRStaticMethodData* static_method, const RTTRType* owner)
{
    auto _log_stack = _logger.stack(u8"export static method {}", static_method->name);

    // basic data
    out.data  = static_method;
    out.owner = owner;

    // export params
    for (const auto* param : static_method->param_data)
    {
        auto& param_binder = out.params_binder.add_default().ref();
        _make_param(param_binder, param);
    }

    // export return
    _make_return(out.return_binder, static_method->ret_type.view());
}
void ScriptBinderManager::_make_prop_getter(ScriptBinderMethod& out, const RTTRMethodData* method, const RTTRType* owner)
{
    // check param count
    if (method->param_data.size() != 0)
    {
        _logger.error(u8"getter param count must be 0, but got {}", method->param_data.size());
        return;
    }

    // check return type
    TypeSignatureTyped<void> void_signature;
    if (method->ret_type.view().equal(void_signature))
    {
        _logger.error(u8"getter return type must not be void");
        return;
    }

    // fill data
    auto& method_data = out.overloads.add_default().ref();
    _make_method(method_data, method, owner);
}
void ScriptBinderManager::_make_prop_setter(ScriptBinderMethod& out, const RTTRMethodData* method, const RTTRType* owner)
{
    // check param count
    if (method->param_data.size() != 1)
    {
        _logger.error(u8"setter param count must be 1, but got {}", method->param_data.size());
        return;
    }

    // check return type
    TypeSignatureTyped<void> void_signature;
    if (!method->ret_type.view().equal(void_signature))
    {
        _logger.error(u8"setter return type must be void");
        return;
    }

    // fill data
    auto& method_data = out.overloads.add_default().ref();
    _make_method(method_data, method, owner);
}
void ScriptBinderManager::_make_static_prop_getter(ScriptBinderStaticMethod& out, const RTTRStaticMethodData* method, const RTTRType* owner)
{
    // check param count
    if (method->param_data.size() != 0)
    {
        _logger.error(u8"getter param count must be 0, but got {}", method->param_data.size());
        return;
    }

    // check return type
    TypeSignatureTyped<void> void_signature;
    if (method->ret_type.view().equal(void_signature))
    {
        _logger.error(u8"getter return type must not be void");
        return;
    }

    // fill data
    auto& method_data = out.overloads.add_default().ref();
    _make_static_method(method_data, method, owner);
}
void ScriptBinderManager::_make_static_prop_setter(ScriptBinderStaticMethod& out, const RTTRStaticMethodData* method, const RTTRType* owner)
{
    // check param count
    if (method->param_data.size() != 1)
    {
        _logger.error(u8"setter param count must be 1, but got {}", method->param_data.size());
        return;
    }

    // check return type
    TypeSignatureTyped<void> void_signature;
    if (!method->ret_type.view().equal(void_signature))
    {
        _logger.error(u8"setter return type must be void");
        return;
    }

    // fill data
    auto& method_data = out.overloads.add_default().ref();
    _make_static_method(method_data, method, owner);
}
void ScriptBinderManager::_make_field(ScriptBinderField& out, const RTTRFieldData* field, const RTTRType* owner)
{
    auto _log_stack = _logger.stack(u8"export field '{}'", field->name);

    // get type signature
    auto signature = field->type.view();

    // export
    ScriptBinderRoot field_binder = {};
    _try_export_field(signature, field_binder);
    // clang-format off
    out = {
        .owner = owner,
        .binder = field_binder,
        .data = field,
    };
    // clang-format on
}
void ScriptBinderManager::_make_static_field(ScriptBinderStaticField& out, const RTTRStaticFieldData* static_field, const RTTRType* owner)
{
    auto _log_stack = _logger.stack(u8"export static field '{}'", static_field->name);

    // get type signature
    auto signature = static_field->type.view();

    // export
    ScriptBinderRoot field_binder = {};
    _try_export_field(signature, field_binder);
    // clang-format off
    out = {
        .owner = owner,
        .binder = field_binder,
        .data = static_field,
    };
    // clang-format on
}
void ScriptBinderManager::_make_param(ScriptBinderParam& out, const RTTRParamData* param)
{
    auto _log_stack = _logger.stack(u8"export param '{}', index {}", param->name, param->index);

    // get type signature
    auto signature = param->type.view();

    // check signature type
    if (!signature.is_type())
    {
        _logger.error(u8"unsupported type");
        return;
    }

    // check pointer level
    if (signature.decayed_pointer_level() > 1)
    {
        _logger.error(u8"pointer level greater than 1");
        return;
    }

    // solve modifier
    bool is_any_ref         = signature.is_any_ref();
    bool is_pointer         = signature.is_pointer();
    bool is_decayed_pointer = is_any_ref || is_pointer;
    bool is_const           = signature.is_const();
    out.is_inout            = (is_any_ref || is_pointer) && !is_const;
    out.is_nullable         = is_pointer;

    // read type id
    skr::GUID param_type_id;
    signature.jump_modifier();
    signature.read_type_id(param_type_id);

    // get binder
    out.binder = get_or_build(param_type_id);

    // solve dynamic stack kind
    out.pass_by_ref = is_decayed_pointer;

    // check bad binder
    if (out.binder.is_empty())
    {
        _logger.error(u8"unsupported type");
        return;
    }

    // check wrap export
    if (out.binder.is_wrap() && !is_decayed_pointer)
    {
        _logger.error(
            u8"export wrap {} as value type",
            out.binder.wrap()->type->name()
        );
        return;
    }
}
void ScriptBinderManager::_make_return(ScriptBinderReturn& out, TypeSignatureView signature)
{
    auto _log_stack = _logger.stack(u8"export return");

    // check signature type
    if (!signature.is_type())
    {
        _logger.error(u8"unsupported type");
        return;
    }

    // check pointer level
    if (signature.decayed_pointer_level() > 1)
    {
        _logger.error(u8"pointer level greater than 1");
        return;
    }

    // solve modifier
    bool is_pointer         = signature.is_pointer();
    bool is_decayed_pointer = signature.is_decayed_pointer();
    out.is_nullable         = is_pointer;
    out.pass_by_ref         = is_decayed_pointer;

    // read type id
    skr::GUID param_type_id;
    signature.jump_modifier();
    signature.read_type_id(param_type_id);

    // get binder
    out.binder = get_or_build(param_type_id);

    // check bad binder
    if (out.binder.is_empty())
    {
        _logger.error(u8"unsupported type");
        return;
    }

    // check wrap export
    if (out.binder.is_wrap() && !is_decayed_pointer)
    {
        _logger.error(
            u8"export wrap type {} as value",
            out.binder.wrap()->type->name()
        );
        return;
    }
}

// checker
void ScriptBinderManager::_try_export_field(TypeSignatureView signature, ScriptBinderRoot& out_binder)
{
    // check type
    if (!signature.is_type())
    {
        _logger.error(u8"unsupported type");
        return;
    }

    // check pointer level
    if (signature.decayed_pointer_level() > 1)
    {
        _logger.error(u8"pointer level greater than 1");
        return;
    }

    if (signature.is_decayed_pointer())
    { // only support pointer type for wrap binder
        if (signature.is_any_ref())
        {
            _logger.error(u8"cannot export reference field");
            return;
        }

        // read type id
        skr::GUID field_type_id;
        signature.jump_modifier();
        signature.read_type_id(field_type_id);

        // export wrap type
        out_binder = get_or_build(field_type_id);
        if (!out_binder.is_wrap())
        {
            _logger.error(u8"cannot export pointer field for non-wrap object");
            return;
        }
    }
    else
    {
        // read type id
        skr::GUID field_type_id;
        signature.jump_modifier();
        signature.read_type_id(field_type_id);

        // export primitive type
        out_binder = get_or_build(field_type_id);
        if (out_binder.is_empty())
        {
            _logger.error(u8"unsupported type");
            return;
        }
        else if (out_binder.is_wrap())
        {
            _logger.error(u8"cannot export wrap object as value type");
        }
    }
}
} // namespace skr