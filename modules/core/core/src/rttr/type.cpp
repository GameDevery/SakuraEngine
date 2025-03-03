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

// module
void Type::set_module(String module)
{
    _module = module;
}
String Type::module() const
{
    return _module;
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
Vector<String> Type::name_space() const
{
    switch (_type_category)
    {
        case ETypeCategory::Primitive:
            return {};
        case ETypeCategory::Record:
            return _record_data.name_space;
        case ETypeCategory::Enum:
            return _enum_data.name_space;
        default:
            SKR_UNREACHABLE_CODE()
            return {};
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
void Type::each_name_space(FunctionRef<void(StringView)> each_func) const
{
    switch (_type_category)
    {
        case ETypeCategory::Primitive:
            break;
        case ETypeCategory::Record:
            for (const String& ns : _record_data.name_space)
            {
                each_func(ns);
            }
            break;
        case ETypeCategory::Enum:
            for (const String& ns : _enum_data.name_space)
            {
                each_func(ns);
            }
            break;
        default:
            SKR_UNREACHABLE_CODE()
            break;
    }
}

// builders
void Type::build_primitive(FunctionRef<void(PrimitiveData* data)> func)
{
    // init
    if (_type_category == ETypeCategory::Invalid)
    {
        new (&_primitive_data) PrimitiveData();
        _type_category = ETypeCategory::Primitive;
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
        _type_category = ETypeCategory::Record;
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
        _type_category = ETypeCategory::Enum;
    }

    // validate
    SKR_ASSERT(_type_category == ETypeCategory::Enum && "Type category mismatch when build enum data");

    // build
    func(&_enum_data);
}

// caster
bool Type::based_on(GUID type_id) const
{
    switch (_type_category)
    {
        case ETypeCategory::Primitive: {
            return (type_id == _primitive_data.type_id);
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
                        return true;;
                    }
                }

                // get base type and continue cast
                for (const auto& base : _record_data.bases_data)
                {
                    auto type = get_type_from_guid(base->type_id);
                    if (type)
                    {
                        if (type->based_on(type_id))
                        {
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
            return false;
    }
}
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

// get dtor
Optional<DtorData> Type::dtor_data() const
{
    switch (_type_category)
    {
        case ETypeCategory::Record:
            return _record_data.dtor_data;
        default:
            return {};
    }
}

// each method & field
void Type::each_bases(FunctionRef<void(const BaseData* base_data, const Type* owner)> each_func, TypeEachConfig config) const
{
    if (_type_category == ETypeCategory::Record)
    {
        // each self
        for (const auto& base : _record_data.bases_data)
        {
            each_func(base, this);
        }

        // each base
        if (config.include_bases)
        {
            for (const auto& base : _record_data.bases_data)
            {
                auto type = get_type_from_guid(base->type_id);
                if (type)
                {
                    type->each_bases(each_func, config);
                }
                else
                {
                    SKR_LOG_FMT_ERROR(u8"Type \"{}\" not found when doing each base from \"{}\"", base->type_id, _record_data.type_id);
                }
            }
        }
    }
}
void Type::each_ctor(FunctionRef<void(const CtorData* ctor)> each_func, TypeEachConfig config) const
{
    if (_type_category == ETypeCategory::Record)
    {
        // each self
        for (const auto& ctor : _record_data.ctor_data)
        {
            each_func(ctor);
        }
    }
}
void Type::each_method(FunctionRef<void(const MethodData* method, const Type* owner)> each_func, TypeEachConfig config) const
{
    if (_type_category == ETypeCategory::Record)
    {
        // each self
        for (const auto& method : _record_data.methods)
        {
            each_func(method, this);
        }

        // each base
        if (config.include_bases)
        {
            for (const auto& base : _record_data.bases_data)
            {
                auto type = get_type_from_guid(base->type_id);
                if (type)
                {
                    type->each_method(each_func, config);
                }
                else
                {
                    SKR_LOG_FMT_ERROR(u8"Type \"{}\" not found when doing each method from \"{}\"", base->type_id, _record_data.type_id);
                }
            }
        }
    }
}
void Type::each_field(FunctionRef<void(const FieldData* field, const Type* owner)> each_func, TypeEachConfig config) const
{
    if (_type_category == ETypeCategory::Record)
    {
        // each self
        for (const auto& field : _record_data.fields)
        {
            each_func(field, this);
        }

        // each base
        if (config.include_bases)
        {
            for (const auto& base : _record_data.bases_data)
            {
                auto type = get_type_from_guid(base->type_id);
                if (type)
                {
                    type->each_field(each_func, config);
                }
                else
                {
                    SKR_LOG_FMT_ERROR(u8"Type \"{}\" not found when doing each method from \"{}\"", base->type_id, _record_data.type_id);
                }
            }
        }
    }
}
void Type::each_static_method(FunctionRef<void(const StaticMethodData* method, const Type* owner)> each_func, TypeEachConfig config) const
{
    if (_type_category == ETypeCategory::Record)
    {
        // each self
        for (const auto& field : _record_data.static_methods)
        {
            each_func(field, this);
        }

        // each base
        if (config.include_bases)
        {
            for (const auto& base : _record_data.bases_data)
            {
                auto type = get_type_from_guid(base->type_id);
                if (type)
                {
                    type->each_static_method(each_func, config);
                }
                else
                {
                    SKR_LOG_FMT_ERROR(u8"Type \"{}\" not found when doing each method from \"{}\"", base->type_id, _record_data.type_id);
                }
            }
        }
    }
}
void Type::each_static_field(FunctionRef<void(const StaticFieldData* field, const Type* owner)> each_func, TypeEachConfig config) const
{
    if (_type_category == ETypeCategory::Record)
    {
        // each self
        for (const auto& field : _record_data.static_fields)
        {
            each_func(field, this);
        }

        // each base
        if (config.include_bases)
        {
            for (const auto& base : _record_data.bases_data)
            {
                auto type = get_type_from_guid(base->type_id);
                if (type)
                {
                    type->each_static_field(each_func, config);
                }
                else
                {
                    SKR_LOG_FMT_ERROR(u8"Type \"{}\" not found when doing each method from \"{}\"", base->type_id, _record_data.type_id);
                }
            }
        }
    }
}
void Type::each_extern_method(FunctionRef<void(const ExternMethodData* method, const Type* owner)> each_func, TypeEachConfig config) const
{
    if (_type_category == ETypeCategory::Record)
    {
        // each self
        for (const auto& method : _record_data.extern_methods)
        {
            each_func(method, this);
        }

        // each base
        if (config.include_bases)
        {
            for (const auto& base : _record_data.bases_data)
            {
                auto type = get_type_from_guid(base->type_id);
                if (type)
                {
                    type->each_extern_method(each_func, config);
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
            each_func(method, this);
        }
    }
}

// find method & field
const CtorData* Type::find_ctor(TypeFindConfig config) const
{
    if (_type_category == ETypeCategory::Record)
    {
        auto find_func = [&](const CtorData* ctor) {
            // filter by signature
            if (config.signature && !ctor->signature_equal(config.signature.value(), config.signature_compare_flag))
            {
                return false;
            }

            return true;
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
const MethodData* Type::find_method(TypeFindConfig config) const
{
    if (_type_category == ETypeCategory::Record)
    {
        auto find_func = [&](const MethodData* method) {
            // filter by name
            if (config.name && method->name != config.name.value())
            {
                return false;
            }

            // filter by signature
            if (config.signature && !method->signature_equal(config.signature.value(), config.signature_compare_flag))
            {
                return false;
            }

            return true;
        };

        // find self
        auto result = _record_data.methods.find_if(find_func);
        if (result)
        {
            return result.ref();
        }

        // find base
        if (config.include_bases)
        {
            for (const auto& base : _record_data.bases_data)
            {
                auto type = get_type_from_guid(base->type_id);
                if (type)
                {
                    auto method = type->find_method(config);
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
const FieldData* Type::find_field(TypeFindConfig config) const
{
    if (_type_category == ETypeCategory::Record)
    {
        auto find_func = [&](const FieldData* field) {
            // filter by name
            if (config.name && field->name != config.name.value())
            {
                return false;
            }

            // filter by signature
            if (config.signature && !field->type.view().equal(config.signature.value(), config.signature_compare_flag))
            {
                return false;
            }

            return true;
        };

        // find self
        auto result = _record_data.fields.find_if(find_func);
        if (result)
        {
            return result.ref();
        }

        // find base
        if (config.include_bases)
        {
            for (const auto& base : _record_data.bases_data)
            {
                auto type = get_type_from_guid(base->type_id);
                if (type)
                {
                    auto field = type->find_field(config);
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
const StaticMethodData* Type::find_static_method(TypeFindConfig config) const
{
    if (_type_category == ETypeCategory::Record)
    {
        auto find_func = [&](const StaticMethodData* method) {
            // filter by name
            if (config.name && method->name != config.name.value())
            {
                return false;
            }

            // filter by signature
            if (config.signature && !method->signature_equal(config.signature.value(), config.signature_compare_flag))
            {
                return false;
            }

            return true;
        };

        // find self
        auto result = _record_data.static_methods.find_if(find_func);
        if (result)
        {
            return result.ref();
        }

        // find base
        if (config.include_bases)
        {
            for (const auto& base : _record_data.bases_data)
            {
                auto type = get_type_from_guid(base->type_id);
                if (type)
                {
                    auto method = type->find_static_method(config);
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
const StaticFieldData* Type::find_static_field(TypeFindConfig config) const
{
    if (_type_category == ETypeCategory::Record)
    {
        auto find_func = [&](const StaticFieldData* field) {
            // filter by name
            if (config.name && field->name != config.name.value())
            {
                return false;
            }

            // filter by signature
            if (config.signature && !field->type.view().equal(config.signature.value(), config.signature_compare_flag))
            {
                return false;
            }

            return true;
        };

        // find self
        auto result = _record_data.static_fields.find_if(find_func);
        if (result)
        {
            return result.ref();
        }

        // find base
        if (config.include_bases)
        {
            for (const auto& base : _record_data.bases_data)
            {
                auto type = get_type_from_guid(base->type_id);
                if (type)
                {
                    auto field = type->find_static_field(config);
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
const ExternMethodData* Type::find_extern_method(TypeFindConfig config) const
{
    auto find_func = [&](const ExternMethodData* method) {
        // filter by name
        if (config.name && method->name != config.name.value())
        {
            return false;
        }

        // filter by signature
        if (config.signature && !method->signature_equal(config.signature.value(), config.signature_compare_flag))
        {
            return false;
        }

        return true;
    };

    if (_type_category == ETypeCategory::Record)
    {
        // find self
        auto result = _record_data.extern_methods.find_if(find_func);
        if (result)
        {
            return result.ref();
        }

        // find base
        if (config.include_bases)
        {
            for (const auto& base : _record_data.bases_data)
            {
                auto type = get_type_from_guid(base->type_id);
                if (type)
                {
                    auto method = type->find_extern_method(config);
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
    else if (_type_category == ETypeCategory::Primitive)
    {
        // find self
        auto result = _primitive_data.extern_methods.find_if(find_func);
        if (result)
        {
            return result.ref();
        }
    }
    else if (_type_category == ETypeCategory::Record)
    {
        // find self
        auto result = _enum_data.extern_methods.find_if(find_func);
        if (result)
        {
            return result.ref();
        }
    }
    return nullptr;
}

// flag & attribute
ERecordFlag Type::record_flag() const
{
    SKR_ASSERT(_type_category == ETypeCategory::Record && "Type category mismatch when get record flag");
    return _record_data.flag;
}
EEnumFlag Type::enum_flag() const
{
    SKR_ASSERT(_type_category == ETypeCategory::Enum && "Type category mismatch when get enum flag");
    return _enum_data.flag;
}
IAttribute* Type::find_attribute(GUID attr_type_id) const
{
    switch (_type_category)
    {
        case ETypeCategory::Record: {
            auto result = _record_data.attributes.find(attr_type_id);
            if (result)
            {
                return result.value();
            }
            break;
        }
        case ETypeCategory::Enum: {
            auto result = _enum_data.attributes.find(attr_type_id);
            if (result)
            {
                return result.value();
            }
            break;
        }
        default:
            break;
    }
    return nullptr;
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
} // namespace skr::rttr