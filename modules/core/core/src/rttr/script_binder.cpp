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
ScriptBinderPrimitive* ScriptBinderManager::_make_primitive(GUID type_id)
{
    switch (type_id.get_hash())
    {
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
    bool failed = false;
    type->each_field([&](const RTTRFieldData* field, const RTTRType* owner_type) {
        if (!flag_all(field->flag, ERTTRFieldFlag::ScriptVisible)) { return; }
        auto& field_data = result.fields.try_add_default(field->name).value();
        if (!_make_field(field_data, field, owner_type))
        {
            failed = true;
        }
        result.fields.add(field->name, field_data);
    });

    // return result
    if (failed)
    {
        return nullptr;
    }
    else
    {
        return SkrNew<ScriptBinderBox>(std::move(result));
    }
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

    bool failed = false;

    // export ctors
    result.is_script_newable = flag_all(type->record_flag(), ERTTRRecordFlag::ScriptNewable);
    if (result.is_script_newable)
    {
        type->each_ctor([&](const RTTRCtorData* ctor) {
            if (!flag_all(ctor->flag, ERTTRCtorFlag::ScriptVisible)) { return; }
            auto& ctor_data = result.ctors.add_default().ref();
            if (!_make_ctor(ctor_data, ctor, type))
            {
                failed = true;
            }
        });
    }

    // export fields
    type->each_field([&](const RTTRFieldData* field, const RTTRType* owner_type) {
        if (!flag_all(field->flag, ERTTRFieldFlag::ScriptVisible)) { return; }
        auto& field_data = result.fields.try_add_default(field->name).value();
        if (!_make_field(field_data, field, owner_type))
        {
            failed = true;
        }
    });

    // export static field
    type->each_static_field([&](const RTTRStaticFieldData* static_field, const RTTRType* owner_type) {
        if (!flag_all(static_field->flag, ERTTRStaticFieldFlag::ScriptVisible)) { return; }
        auto& static_field_data = result.static_fields.try_add_default(static_field->name).value();
        if (!_make_static_field(static_field_data, static_field, owner_type))
        {
            failed = true;
        }
    });

    // export methods
    type->each_method([&](const RTTRMethodData* method, const RTTRType* owner_type) {
        if (!flag_all(method->flag, ERTTRMethodFlag::ScriptVisible)) { return; }
        auto& method_data   = result.methods.try_add_default(method->name).value();
        auto& overload_data = method_data.overloads.add_default().ref();
        if (!_make_method(overload_data, method, owner_type))
        {
            failed = true;
        }
    });

    // export static methods
    type->each_static_method([&](const RTTRStaticMethodData* method, const RTTRType* owner_type) {
        if (!flag_all(method->flag, ERTTRStaticMethodFlag::ScriptVisible)) { return; }
        auto& static_method_data = result.static_methods.try_add_default(method->name).value();
        auto& overload_data      = static_method_data.overloads.add_default().ref();
        if (!_make_static_method(overload_data, method, owner_type))
        {
            failed = true;
        }
    });

    // return result
    if (failed)
    {
        return nullptr;
    }
    else
    {
        return SkrNew<ScriptBinderWrap>(std::move(result));
    }
}

// make nested binder
bool ScriptBinderManager::_make_ctor(ScriptBinderCtor& out, const RTTRCtorData* ctor, const RTTRType* owner)
{
    // basic data
    out.data = ctor;

    bool failed = false;

    // export params
    for (const auto* param : ctor->param_data)
    {
        auto& param_binder = out.params_binder.add_default().ref();
        failed |= !_make_param(param_binder, u8"[Ctor]", param, owner);
    }

    return !failed;
}
bool ScriptBinderManager::_make_method(ScriptBinderMethod::Overload& out, const RTTRMethodData* method, const RTTRType* owner)
{
    // basic data
    out.data  = method;
    out.owner = owner;

    bool failed = false;

    // export params
    for (const auto* param : method->param_data)
    {
        auto& param_binder = out.params_binder.add_default().ref();
        failed |= !_make_param(param_binder, method->name, param, owner);
    }

    // export return
    failed |= !_make_return(out.return_binder, method->name, method->ret_type.view(), owner);

    return !failed;
}
bool ScriptBinderManager::_make_static_method(ScriptBinderStaticMethod::Overload& out, const RTTRStaticMethodData* static_method, const RTTRType* owner)
{
    // basic data
    out.data  = static_method;
    out.owner = owner;

    bool failed = false;

    // export params
    for (const auto* param : static_method->param_data)
    {
        auto& param_binder = out.params_binder.add_default().ref();
        failed |= !_make_param(param_binder, static_method->name, param, owner);
    }

    // export return
    failed |= !_make_return(out.return_binder, static_method->name, static_method->ret_type.view(), owner);

    return !failed;
}
bool ScriptBinderManager::_make_field(ScriptBinderField& out, const RTTRFieldData* field, const RTTRType* owner)
{
    // get type signature
    auto signature = field->type.view();

    // export
    ScriptBinderRoot field_binder = {};
    auto             err_code     = _try_export_field(signature, field_binder);
    if (err_code == EScriptBindFailed::None)
    {
        // clang-format off
        out = {
            .owner = owner,
            .binder = field_binder,
            .data = field,
        };
        // clang-format on
        return true;
    }
    else
    {
        _err_field(field->name, owner->name(), err_code);
        return false;
    }
}
bool ScriptBinderManager::_make_static_field(ScriptBinderStaticField& out, const RTTRStaticFieldData* static_field, const RTTRType* owner)
{
    // get type signature
    auto signature = static_field->type.view();

    // export
    ScriptBinderRoot field_binder = {};
    auto             err_code     = _try_export_field(signature, field_binder);
    if (err_code == EScriptBindFailed::None)
    {
        // clang-format off
        out = {
            .owner = owner,
            .binder = field_binder,
            .data = static_field,
        };
        // clang-format on
        return true;
    }
    else
    {
        _err_field(static_field->name, owner->name(), err_code);
        return false;
    }
}
bool ScriptBinderManager::_make_param(ScriptBinderParam& out, StringView method_name, const RTTRParamData* param, const RTTRType* owner)
{
    // get type signature
    auto signature = param->type.view();

    // check signature type
    if (!signature.is_type())
    {
        _err_param(
            param->name,
            method_name,
            EScriptBindFailed::Unknown
        );
        return false;
    }

    // check pointer level
    if (signature.decayed_pointer_level() > 1)
    {
        _err_param(
            param->name,
            method_name,
            EScriptBindFailed::ExportPointerLevelGreaterThanOne
        );
        return false;
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
        _err_param(
            param->name,
            method_name,
            EScriptBindFailed::Unknown
        );
        return false;
    }

    // check wrap export
    if (out.binder.is_wrap() && !is_decayed_pointer)
    {
        _err_param(
            param->name,
            method_name,
            EScriptBindFailed::ExportWrapByValue
        );
        return false;
    }

    return true;
}
bool ScriptBinderManager::_make_return(ScriptBinderReturn& out, StringView method_name, TypeSignatureView signature, const RTTRType* owner)
{
    // check signature type
    if (!signature.is_type())
    {
        _err_return(
            method_name,
            EScriptBindFailed::Unknown
        );
        return false;
    }

    // check pointer level
    if (signature.decayed_pointer_level() > 1)
    {
        _err_return(
            method_name,
            EScriptBindFailed::ExportPointerLevelGreaterThanOne
        );
        return false;
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
        _err_return(
            method_name,
            EScriptBindFailed::Unknown
        );
        return false;
    }

    // check wrap export
    if (out.binder.is_wrap() && !is_decayed_pointer)
    {
        _err_return(
            method_name,
            EScriptBindFailed::ExportWrapByValue
        );
        return false;
    }

    return true;
}

// checker
EScriptBindFailed ScriptBinderManager::_try_export_field(TypeSignatureView signature, ScriptBinderRoot& out_binder)
{
    // check type
    if (!signature.is_type())
    {
        return EScriptBindFailed::Unknown;
    }

    // check pointer level
    if (signature.decayed_pointer_level() > 1)
    {
        return EScriptBindFailed::ExportPointerLevelGreaterThanOne;
    }

    if (signature.is_decayed_pointer())
    { // only support pointer type for wrap binder
        if (signature.is_any_ref())
        {
            return EScriptBindFailed::ExportReferenceField;
        }

        // read type id
        skr::GUID field_type_id;
        signature.jump_modifier();
        signature.read_type_id(field_type_id);

        // export wrap type
        ScriptBinderRoot out_binder = get_or_build(field_type_id);
        if (!out_binder.is_wrap())
        {
            return EScriptBindFailed::ExportPointerFieldOfPrimitiveOrBox;
        }
    }
    else
    {
        // read type id
        skr::GUID field_type_id;
        signature.jump_modifier();
        signature.read_type_id(field_type_id);

        // export primitive type
        ScriptBinderRoot field_binder = get_or_build(field_type_id);
        if (field_binder.is_empty())
        {
            return EScriptBindFailed::Unknown;
        }
        else if (field_binder.is_wrap())
        {
            return EScriptBindFailed::ExportWrapByValue;
        }
    }
    return EScriptBindFailed::None;
}

// error helper
StringView ScriptBinderManager::_err_string(EScriptBindFailed err)
{
    switch (err)
    {
    case EScriptBindFailed::None:
        return {};
    case EScriptBindFailed::ExportReferenceField:
        return u8"'T&/const T&' is not allowed";
    case EScriptBindFailed::ExportPointerFieldOfPrimitiveOrBox:
        return u8"'T*/const T*' field must be wrap type";
    case EScriptBindFailed::ExportWrapByValue:
        return u8"export 'T' of wrap type is not allowed, must be 'T*/const T*'";
    case EScriptBindFailed::ExportPointerLevelGreaterThanOne:
        return u8"cannot export pointer level greater than one, e.g. T**/T*&/const T*&";
    default:
        return u8"failed to export type";
    }
}
void ScriptBinderManager::_err_field(StringView field_name, StringView owner_name, EScriptBindFailed err)
{
    if (err == EScriptBindFailed::None) { return; }
    SKR_LOG_FMT_ERROR(
        u8"failed export field {}::{} use box mode\n\treason: {}",
        field_name,
        owner_name,
        _err_string(err)
    );
}
void ScriptBinderManager::_err_param(StringView param_name, StringView method_name, EScriptBindFailed err)
{
    if (err == EScriptBindFailed::None) { return; }
    SKR_LOG_FMT_ERROR(
        u8"failed to export param {}, method {}\n\treason: {}",
        param_name,
        method_name,
        _err_string(err)
    );
}
void ScriptBinderManager::_err_return(StringView method_name, EScriptBindFailed err)
{
    if (err == EScriptBindFailed::None) { return; }
    SKR_LOG_FMT_ERROR(
        u8"failed to export return type of method {}\n\treason: {}",
        method_name,
        _err_string(err)
    );
}
} // namespace skr