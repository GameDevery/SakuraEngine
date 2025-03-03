#include "SkrRTTR/type.hpp"
#include "SkrCore/log.hpp"

namespace skr::rttr
{
// ctor & dtor
Type::Type()
{
}
Type::~Type()
{
    switch (_type_category)
    {
        case ETypeCategory::Invalid:
            break;
        case ETypeCategory::Primitive:
            _primitive_data.~PrimitiveData();
            break;
        case ETypeCategory::Record:
            _record_data.~RecordData();
            break;
        case ETypeCategory::Enum:
            _enum_data.~EnumData();
            break;
        default:
            SKR_UNREACHABLE_CODE()
            break;
    }
}

// basic getter
ETypeCategory Type::type_category() const
{
    return _type_category;
}
const skr::String& Type::name() const
{
    switch (_type_category)
    {
        case ETypeCategory::Primitive:
            return _primitive_data.name;
        case ETypeCategory::Record:
            return _record_data.name;
        case ETypeCategory::Enum:
            return _enum_data.name;
        default:
            SKR_UNREACHABLE_CODE()
            return _primitive_data.name;
    }
}
GUID Type::type_id() const
{
    switch (_type_category)
    {
        case ETypeCategory::Primitive:
            return _primitive_data.type_id;
        case ETypeCategory::Record:
            return _record_data.type_id;
        case ETypeCategory::Enum:
            return _enum_data.type_id;
        default:
            SKR_UNREACHABLE_CODE()
            return _primitive_data.type_id;
    }
}
size_t Type::size() const
{
    switch (_type_category)
    {
        case ETypeCategory::Primitive:
            return _primitive_data.size;
        case ETypeCategory::Record:
            return _record_data.size;
        case ETypeCategory::Enum:
            return _enum_data.size;
        default:
            SKR_UNREACHABLE_CODE()
            return _primitive_data.size;
    }
}
size_t Type::alignment() const
{
    switch (_type_category)
    {
        case ETypeCategory::Primitive:
            return _primitive_data.alignment;
        case ETypeCategory::Record:
            return _record_data.alignment;
        case ETypeCategory::Enum:
            return _enum_data.alignment;
        default:
            SKR_UNREACHABLE_CODE()
            return _primitive_data.alignment;
    }
}

// builders
void Type::build_primitive(FunctionRef<void(PrimitiveData* data)> func)
{
    // init
    if (_type_category == ETypeCategory::Invalid)
    {
        new (&_primitive_data) PrimitiveData();
    }

    // validate
    SKR_ASSERT(_type_category == ETypeCategory::Primitive && "Type category mismatch when build primitive data");

    // build
    func(&_primitive_data);
}
void Type::build_record(FunctionRef<void(RecordData* data)> func)
{
    // init
    if (_type_category == ETypeCategory::Invalid)
    {
        new (&_record_data) RecordData();
    }

    // validate
    SKR_ASSERT(_type_category == ETypeCategory::Record && "Type category mismatch when build record data");

    // build
    func(&_record_data);
}
void Type::build_enum(FunctionRef<void(EnumData* data)> func)
{
    // init
    if (_type_category == ETypeCategory::Invalid)
    {
        new (&_enum_data) EnumData();
    }

    // validate
    SKR_ASSERT(_type_category == ETypeCategory::Enum && "Type category mismatch when build enum data");

    // build
    func(&_enum_data);
}

// caster
void* Type::cast_to(GUID type_id, void* p) const
{
    switch (_type_category)
    {
        case ETypeCategory::Primitive: {
            return (type_id == _primitive_data.type_id) ? p : nullptr;
        }
        case ETypeCategory::Record: {
            if (type_id == _record_data.type_id)
            {
                return p;
            }
            else
            {
                // find base and cast
                for (const auto& base : _record_data.bases_data)
                {
                    if (type_id == base->type_id)
                    {
                        return base->cast_to_base(p);
                    }
                }

                // get base type and continue cast
                for (const auto& base : _record_data.bases_data)
                {
                    auto type = get_type_from_guid(base->type_id);
                    if (type)
                    {
                        auto casted = type->cast_to(type_id, p);
                        if (casted)
                        {
                            return casted;
                        }
                    }
                    else
                    {
                        SKR_LOG_FMT_ERROR(u8"Type \"{}\" not found when doing cast from \"{}\"", type_id, _record_data.type_id);
                    }
                }
            }
        }
        case ETypeCategory::Enum: {
            return (type_id == _enum_data.type_id || type_id == _enum_data.underlying_type_id) ? p : nullptr;
        }
        default:
            SKR_UNREACHABLE_CODE()
            return nullptr;
    }
}
TypeCaster Type::caster(GUID type_id) const
{
    TypeCaster result;
    _build_caster(result, type_id);
    return result;
}

// helpers
bool Type::_build_caster(TypeCaster& caster, const GUID& type_id) const
{
    switch (_type_category)
    {
        case ETypeCategory::Primitive: {
            return type_id == _primitive_data.type_id;
        }
        case ETypeCategory::Record: {
            if (type_id == _record_data.type_id)
            {
                return true;
            }
            else
            {
                // find base and cast
                for (const auto& base : _record_data.bases_data)
                {
                    if (type_id == base->type_id)
                    {
                        caster.cast_funcs.add(base->cast_to_base);
                        return true;
                    }
                }

                // get base type and continue cast
                for (const auto& base : _record_data.bases_data)
                {
                    auto type = get_type_from_guid(base->type_id);
                    if (type)
                    {
                        if (_build_caster(caster, type_id))
                        {
                            caster.cast_funcs.add(base->cast_to_base);
                            return true;
                        }
                    }
                    else
                    {
                        SKR_LOG_FMT_ERROR(u8"Type \"{}\" not found when doing cast from \"{}\"", type_id, _record_data.type_id);
                    }
                }
            }
        }
        case ETypeCategory::Enum: {
            return (type_id == _enum_data.type_id || type_id == _enum_data.underlying_type_id);
        }
        default:
            SKR_UNREACHABLE_CODE()
            break;
    }
    return false;
}

// each
void Type::each_ctor(FunctionRef<void(const CtorData* ctor)> each_func, bool include_base) const
{
    if (_type_category == ETypeCategory::Record)
    {
        // each self
        for (const auto& ctor : _record_data.ctor_data)
        {
            each_func(ctor);
        }

        // each base
        if (include_base)
        {
            for (const auto& base : _record_data.bases_data)
            {
                auto type = get_type_from_guid(base->type_id);
                if (type)
                {
                    type->each_ctor(each_func, include_base);
                }
                else
                {
                    SKR_LOG_FMT_ERROR(u8"Type \"{}\" not found when doing each ctor from \"{}\"", base->type_id, _record_data.type_id);
                }
            }
        }
    }
}
void Type::each_method(FunctionRef<void(const MethodData* method)> each_func, bool include_base) const
{
    if (_type_category == ETypeCategory::Record)
    {
        // each self
        for (const auto& method : _record_data.methods)
        {
            each_func(method);
        }

        // each base
        if (include_base)
        {
            for (const auto& base : _record_data.bases_data)
            {
                auto type = get_type_from_guid(base->type_id);
                if (type)
                {
                    type->each_method(each_func, include_base);
                }
                else
                {
                    SKR_LOG_FMT_ERROR(u8"Type \"{}\" not found when doing each method from \"{}\"", base->type_id, _record_data.type_id);
                }
            }
        }
    }
}
void Type::each_field(FunctionRef<void(const FieldData* field)> each_func, bool include_base) const
{
    if (_type_category == ETypeCategory::Record)
    {
        // each self
        for (const auto& field : _record_data.fields)
        {
            each_func(field);
        }

        // each base
        if (include_base)
        {
            for (const auto& base : _record_data.bases_data)
            {
                auto type = get_type_from_guid(base->type_id);
                if (type)
                {
                    type->each_field(each_func, include_base);
                }
                else
                {
                    SKR_LOG_FMT_ERROR(u8"Type \"{}\" not found when doing each method from \"{}\"", base->type_id, _record_data.type_id);
                }
            }
        }
    }
}
void Type::each_static_method(FunctionRef<void(const StaticMethodData* method)> each_func, bool include_base) const
{
    if (_type_category == ETypeCategory::Record)
    {
        // each self
        for (const auto& field : _record_data.static_methods)
        {
            each_func(field);
        }

        // each base
        if (include_base)
        {
            for (const auto& base : _record_data.bases_data)
            {
                auto type = get_type_from_guid(base->type_id);
                if (type)
                {
                    type->each_static_method(each_func, include_base);
                }
                else
                {
                    SKR_LOG_FMT_ERROR(u8"Type \"{}\" not found when doing each method from \"{}\"", base->type_id, _record_data.type_id);
                }
            }
        }
    }
}
void Type::each_static_field(FunctionRef<void(const StaticFieldData* field)> each_func, bool include_base) const
{
    if (_type_category == ETypeCategory::Record)
    {
        // each self
        for (const auto& field : _record_data.static_fields)
        {
            each_func(field);
        }

        // each base
        if (include_base)
        {
            for (const auto& base : _record_data.bases_data)
            {
                auto type = get_type_from_guid(base->type_id);
                if (type)
                {
                    type->each_static_field(each_func, include_base);
                }
                else
                {
                    SKR_LOG_FMT_ERROR(u8"Type \"{}\" not found when doing each method from \"{}\"", base->type_id, _record_data.type_id);
                }
            }
        }
    }
}
void Type::each_extern_method(FunctionRef<void(const ExternMethodData* method)> each_func, bool include_base) const
{
    if (_type_category == ETypeCategory::Record)
    {
        // each self
        for (const auto& method : _record_data.extern_methods)
        {
            each_func(method);
        }

        // each base
        if (include_base)
        {
            for (const auto& base : _record_data.bases_data)
            {
                auto type = get_type_from_guid(base->type_id);
                if (type)
                {
                    type->each_extern_method(each_func, include_base);
                }
                else
                {
                    SKR_LOG_FMT_ERROR(u8"Type \"{}\" not found when doing each method from \"{}\"", base->type_id, _record_data.type_id);
                }
            }
        }
    }
    else if (_type_category == ETypeCategory::Primitive)
    {
        // each self
        for (const auto& method : _primitive_data.extern_methods)
        {
            each_func(method);
        }
    }
}

// find
const CtorData* Type::find_ctor(TypeSignatureView signature, ETypeSignatureCompareFlag flag) const
{
    if (_type_category == ETypeCategory::Record)
    {
        auto find_func = [&](const CtorData* ctor) {
            return ctor->signature_equal(signature, flag);
        };

        // find self
        auto result = _record_data.ctor_data.find_if(find_func);
        if (result)
        {
            return result.ref();
        }
    }
    return nullptr;
}
const MethodData* Type::find_method(StringView name, TypeSignatureView signature, ETypeSignatureCompareFlag flag, bool include_base) const
{
    if (_type_category == ETypeCategory::Record)
    {
        auto find_func = [&](const MethodData* method) {
            return method->name == name && method->signature_equal(signature, flag);
        };

        // find self
        auto result = _record_data.methods.find_if(find_func);
        if (result)
        {
            return result.ref();
        }
        
        // find base
        if (include_base)
        {
            for (const auto& base : _record_data.bases_data)
            {
                auto type = get_type_from_guid(base->type_id);
                if (type)
                {
                    auto method = type->find_method(name, signature, flag, include_base);
                    if (method)
                    {
                        return method;
                    }
                }
                else
                {
                    SKR_LOG_FMT_ERROR(u8"Type \"{}\" not found when doing find method from \"{}\"", base->type_id, _record_data.type_id);
                }
            }
        }
    }
    return nullptr;
}
const FieldData* Type::find_field(StringView name, TypeSignatureView signature, ETypeSignatureCompareFlag flag, bool include_base) const
{
    if (_type_category == ETypeCategory::Record)
    {
        auto find_func = [&](const FieldData* field) {
            return field->name == name && field->type.view().equal(signature, flag);
        };

        // find self
        auto result = _record_data.fields.find_if(find_func);
        if (result)
        {
            return result.ref();
        }

        // find base
        if (include_base)
        {
            for (const auto& base : _record_data.bases_data)
            {
                auto type = get_type_from_guid(base->type_id);
                if (type)
                {
                    auto field = type->find_field(name, signature, flag, include_base);
                    if (field)
                    {
                        return field;
                    }
                }
                else
                {
                    SKR_LOG_FMT_ERROR(u8"Type \"{}\" not found when doing find field from \"{}\"", base->type_id, _record_data.type_id);
                }
            }
        }
    }
    return nullptr;
}
const StaticMethodData* Type::find_static_method(StringView name, TypeSignatureView signature, ETypeSignatureCompareFlag flag, bool include_base) const
{
    if (_type_category == ETypeCategory::Record)
    {
        auto find_func = [&](const StaticMethodData* method) {
            return method->name == name && method->signature_equal(signature, flag);
        };

        // find self
        auto result = _record_data.static_methods.find_if(find_func);
        if (result)
        {
            return result.ref();
        }

        // find base
        if (include_base)
        {
            for (const auto& base : _record_data.bases_data)
            {
                auto type = get_type_from_guid(base->type_id);
                if (type)
                {
                    auto method = type->find_static_method(name, signature, flag, include_base);
                    if (method)
                    {
                        return method;
                    }
                }
                else
                {
                    SKR_LOG_FMT_ERROR(u8"Type \"{}\" not found when doing find static method from \"{}\"", base->type_id, _record_data.type_id);
                }
            }
        }
    }
    return nullptr;
}
const StaticFieldData* Type::find_static_field(StringView name, TypeSignatureView signature, ETypeSignatureCompareFlag flag, bool include_base) const
{
    if (_type_category == ETypeCategory::Record)
    {
        auto find_func = [&](const StaticFieldData* field) {
            return field->name == name && field->type.view().equal(signature, flag);
        };

        // find self
        auto result = _record_data.static_fields.find_if(find_func);
        if (result)
        {
            return result.ref();
        }

        // find base
        if (include_base)
        {
            for (const auto& base : _record_data.bases_data)
            {
                auto type = get_type_from_guid(base->type_id);
                if (type)
                {
                    auto field = type->find_static_field(name, signature, flag, include_base);
                    if (field)
                    {
                        return field;
                    }
                }
                else
                {
                    SKR_LOG_FMT_ERROR(u8"Type \"{}\" not found when doing find static field from \"{}\"", base->type_id, _record_data.type_id);
                }
            }
        }
    }
    return nullptr;
}
const ExternMethodData* Type::find_extern_method(StringView name, TypeSignatureView signature, ETypeSignatureCompareFlag flag, bool include_base) const
{
    if (_type_category == ETypeCategory::Record)
    {
        auto find_func = [&](const ExternMethodData* method) {
            return method->name == name && method->signature_equal(signature, flag);
        };

        // find self
        auto result = _record_data.extern_methods.find_if(find_func);
        if (result)
        {
            return result.ref();
        }

        // find base
        if (include_base)
        {
            for (const auto& base : _record_data.bases_data)
            {
                auto type = get_type_from_guid(base->type_id);
                if (type)
                {
                    auto method = type->find_extern_method(name, signature, flag, include_base);
                    if (method)
                    {
                        return method;
                    }
                }
                else
                {
                    SKR_LOG_FMT_ERROR(u8"Type \"{}\" not found when doing find extern method from \"{}\"", base->type_id, _record_data.type_id);
                }
            }
        }
    }
    return nullptr;
}

} // namespace skr::rttr