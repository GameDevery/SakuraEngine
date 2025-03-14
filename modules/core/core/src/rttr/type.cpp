#include "SkrRTTR/type.hpp"
#include "SkrCore/log.hpp"

namespace skr
{
// ctor & dtor
RTTRType::RTTRType()
{
}
RTTRType::~RTTRType()
{
    switch (_type_category)
    {
    case ERTTRTypeCategory::Invalid:
        break;
    case ERTTRTypeCategory::Primitive:
        _primitive_data.~PrimitiveData();
        break;
    case ERTTRTypeCategory::Record:
        _record_data.~RecordData();
        break;
    case ERTTRTypeCategory::Enum:
        _enum_data.~EnumData();
        break;
    default:
        SKR_UNREACHABLE_CODE()
        break;
    }
}

// module
void RTTRType::set_module(String module)
{
    _module = module;
}
String RTTRType::module() const
{
    return _module;
}

// basic getter
ERTTRTypeCategory RTTRType::type_category() const
{
    return _type_category;
}
const skr::String& RTTRType::name() const
{
    switch (_type_category)
    {
    case ERTTRTypeCategory::Primitive:
        return _primitive_data.name;
    case ERTTRTypeCategory::Record:
        return _record_data.name;
    case ERTTRTypeCategory::Enum:
        return _enum_data.name;
    default:
        SKR_UNREACHABLE_CODE()
        return _primitive_data.name;
    }
}
Vector<String> RTTRType::name_space() const
{
    switch (_type_category)
    {
    case ERTTRTypeCategory::Primitive:
        return {};
    case ERTTRTypeCategory::Record:
        return _record_data.name_space;
    case ERTTRTypeCategory::Enum:
        return _enum_data.name_space;
    default:
        SKR_UNREACHABLE_CODE()
        return {};
    }
}
GUID RTTRType::type_id() const
{
    switch (_type_category)
    {
    case ERTTRTypeCategory::Primitive:
        return _primitive_data.type_id;
    case ERTTRTypeCategory::Record:
        return _record_data.type_id;
    case ERTTRTypeCategory::Enum:
        return _enum_data.type_id;
    default:
        SKR_UNREACHABLE_CODE()
        return _primitive_data.type_id;
    }
}
size_t RTTRType::size() const
{
    switch (_type_category)
    {
    case ERTTRTypeCategory::Primitive:
        return _primitive_data.size;
    case ERTTRTypeCategory::Record:
        return _record_data.size;
    case ERTTRTypeCategory::Enum:
        return _enum_data.size;
    default:
        SKR_UNREACHABLE_CODE()
        return _primitive_data.size;
    }
}
size_t RTTRType::alignment() const
{
    switch (_type_category)
    {
    case ERTTRTypeCategory::Primitive:
        return _primitive_data.alignment;
    case ERTTRTypeCategory::Record:
        return _record_data.alignment;
    case ERTTRTypeCategory::Enum:
        return _enum_data.alignment;
    default:
        SKR_UNREACHABLE_CODE()
        return _primitive_data.alignment;
    }
}
void RTTRType::each_name_space(FunctionRef<void(StringView)> each_func) const
{
    switch (_type_category)
    {
    case ERTTRTypeCategory::Primitive:
        break;
    case ERTTRTypeCategory::Record:
        for (const String& ns : _record_data.name_space)
        {
            each_func(ns);
        }
        break;
    case ERTTRTypeCategory::Enum:
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
void RTTRType::build_primitive(FunctionRef<void(rttr::PrimitiveData* data)> func)
{
    // init
    if (_type_category == ERTTRTypeCategory::Invalid)
    {
        new (&_primitive_data) rttr::PrimitiveData();
        _type_category = ERTTRTypeCategory::Primitive;
    }

    // validate
    SKR_ASSERT(_type_category == ERTTRTypeCategory::Primitive && "Type category mismatch when build primitive data");

    // build
    func(&_primitive_data);
}
void RTTRType::build_record(FunctionRef<void(rttr::RecordData* data)> func)
{
    // init
    if (_type_category == ERTTRTypeCategory::Invalid)
    {
        new (&_record_data) rttr::RecordData();
        _type_category = ERTTRTypeCategory::Record;
    }

    // validate
    SKR_ASSERT(_type_category == ERTTRTypeCategory::Record && "Type category mismatch when build record data");

    // build
    func(&_record_data);
}
void RTTRType::build_enum(FunctionRef<void(rttr::EnumData* data)> func)
{
    // init
    if (_type_category == ERTTRTypeCategory::Invalid)
    {
        new (&_enum_data) rttr::EnumData();
        _type_category = ERTTRTypeCategory::Enum;
    }

    // validate
    SKR_ASSERT(_type_category == ERTTRTypeCategory::Enum && "Type category mismatch when build enum data");

    // build
    func(&_enum_data);
}

// caster
bool RTTRType::based_on(GUID type_id) const
{
    switch (_type_category)
    {
    case ERTTRTypeCategory::Primitive: {
        return (type_id == _primitive_data.type_id);
    }
    case ERTTRTypeCategory::Record: {
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
                    return true;
                    ;
                }
            }

            // get base type and continue cast
            for (const auto& base : _record_data.bases_data)
            {
                auto type = rttr::get_type_from_guid(base->type_id);
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

            return false;
        }
    }
    case ERTTRTypeCategory::Enum: {
        return (type_id == _enum_data.type_id || type_id == _enum_data.underlying_type_id);
    }
    default:
        SKR_UNREACHABLE_CODE()
        return false;
    }
}
void* RTTRType::cast_to_base(GUID type_id, void* p) const
{
    switch (_type_category)
    {
    case ERTTRTypeCategory::Primitive: {
        return (type_id == _primitive_data.type_id) ? p : nullptr;
    }
    case ERTTRTypeCategory::Record: {
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
                auto type = rttr::get_type_from_guid(base->type_id);
                if (type)
                {
                    auto casted = type->cast_to_base(type_id, p);
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
            return nullptr;
        }
    }
    case ERTTRTypeCategory::Enum: {
        return (type_id == _enum_data.type_id || type_id == _enum_data.underlying_type_id) ? p : nullptr;
    }
    default:
        SKR_UNREACHABLE_CODE()
        return nullptr;
    }
}
RTTRTypeCaster RTTRType::caster_to_base(GUID type_id) const
{
    RTTRTypeCaster result;
    _build_caster(result, type_id);
    return result;
}

// get dtor
Optional<rttr::DtorData> RTTRType::dtor_data() const
{
    switch (_type_category)
    {
    case ERTTRTypeCategory::Record:
        return _record_data.dtor_data;
    default:
        return {};
    }
}

// each method & field
void RTTRType::each_bases(FunctionRef<void(const rttr::BaseData* base_data, const RTTRType* owner)> each_func, RTTRTypeEachConfig config) const
{
    if (_type_category == ERTTRTypeCategory::Record)
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
                auto type = rttr::get_type_from_guid(base->type_id);
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
void RTTRType::each_ctor(FunctionRef<void(const rttr::CtorData* ctor)> each_func, RTTRTypeEachConfig config) const
{
    if (_type_category == ERTTRTypeCategory::Record)
    {
        // each self
        for (const auto& ctor : _record_data.ctor_data)
        {
            each_func(ctor);
        }
    }
}
void RTTRType::each_method(FunctionRef<void(const rttr::MethodData* method, const RTTRType* owner)> each_func, RTTRTypeEachConfig config) const
{
    if (_type_category == ERTTRTypeCategory::Record)
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
                auto type = rttr::get_type_from_guid(base->type_id);
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
void RTTRType::each_field(FunctionRef<void(const rttr::FieldData* field, const RTTRType* owner)> each_func, RTTRTypeEachConfig config) const
{
    if (_type_category == ERTTRTypeCategory::Record)
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
                auto type = rttr::get_type_from_guid(base->type_id);
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
void RTTRType::each_static_method(FunctionRef<void(const rttr::StaticMethodData* method, const RTTRType* owner)> each_func, RTTRTypeEachConfig config) const
{
    if (_type_category == ERTTRTypeCategory::Record)
    {
        // each self
        for (const auto& method : _record_data.static_methods)
        {
            each_func(method, this);
        }

        // each base
        if (config.include_bases)
        {
            for (const auto& base : _record_data.bases_data)
            {
                auto type = rttr::get_type_from_guid(base->type_id);
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
void RTTRType::each_static_field(FunctionRef<void(const rttr::StaticFieldData* field, const RTTRType* owner)> each_func, RTTRTypeEachConfig config) const
{
    if (_type_category == ERTTRTypeCategory::Record)
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
                auto type = rttr::get_type_from_guid(base->type_id);
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
void RTTRType::each_extern_method(FunctionRef<void(const rttr::ExternMethodData* method, const RTTRType* owner)> each_func, RTTRTypeEachConfig config) const
{
    if (_type_category == ERTTRTypeCategory::Record)
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
                auto type = rttr::get_type_from_guid(base->type_id);
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
    else if (_type_category == ERTTRTypeCategory::Primitive)
    {
        // each self
        for (const auto& method : _primitive_data.extern_methods)
        {
            each_func(method, this);
        }
    }
}

// find method & field
const rttr::CtorData* RTTRType::find_ctor(RTTRTypeFindConfig config) const
{
    if (_type_category == ERTTRTypeCategory::Record)
    {
        auto find_func = [&](const rttr::CtorData* ctor) {
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
const rttr::MethodData* RTTRType::find_method(RTTRTypeFindConfig config) const
{
    if (_type_category == ERTTRTypeCategory::Record)
    {
        auto find_func = [&](const rttr::MethodData* method) {
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
                auto type = rttr::get_type_from_guid(base->type_id);
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
const rttr::FieldData* RTTRType::find_field(RTTRTypeFindConfig config) const
{
    if (_type_category == ERTTRTypeCategory::Record)
    {
        auto find_func = [&](const rttr::FieldData* field) {
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
                auto type = rttr::get_type_from_guid(base->type_id);
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
const rttr::StaticMethodData* RTTRType::find_static_method(RTTRTypeFindConfig config) const
{
    if (_type_category == ERTTRTypeCategory::Record)
    {
        auto find_func = [&](const rttr::StaticMethodData* method) {
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
                auto type = rttr::get_type_from_guid(base->type_id);
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
const rttr::StaticFieldData* RTTRType::find_static_field(RTTRTypeFindConfig config) const
{
    if (_type_category == ERTTRTypeCategory::Record)
    {
        auto find_func = [&](const rttr::StaticFieldData* field) {
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
                auto type = rttr::get_type_from_guid(base->type_id);
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
const rttr::ExternMethodData* RTTRType::find_extern_method(RTTRTypeFindConfig config) const
{
    auto find_func = [&](const rttr::ExternMethodData* method) {
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

    if (_type_category == ERTTRTypeCategory::Record)
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
                auto type = rttr::get_type_from_guid(base->type_id);
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
    else if (_type_category == ERTTRTypeCategory::Primitive)
    {
        // find self
        auto result = _primitive_data.extern_methods.find_if(find_func);
        if (result)
        {
            return result.ref();
        }
    }
    else if (_type_category == ERTTRTypeCategory::Record)
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
rttr::ERecordFlag RTTRType::record_flag() const
{
    SKR_ASSERT(_type_category == ERTTRTypeCategory::Record && "Type category mismatch when get record flag");
    return _record_data.flag;
}
rttr::EEnumFlag RTTRType::enum_flag() const
{
    SKR_ASSERT(_type_category == ERTTRTypeCategory::Enum && "Type category mismatch when get enum flag");
    return _enum_data.flag;
}
rttr::IAttribute* RTTRType::find_attribute(GUID attr_type_id) const
{
    switch (_type_category)
    {
    case ERTTRTypeCategory::Record: {
        auto result = _record_data.attributes.find(attr_type_id);
        if (result)
        {
            return result.value();
        }
        break;
    }
    case ERTTRTypeCategory::Enum: {
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
bool RTTRType::_build_caster(RTTRTypeCaster& caster, const GUID& type_id) const
{
    switch (_type_category)
    {
    case ERTTRTypeCategory::Primitive: {
        return type_id == _primitive_data.type_id;
    }
    case ERTTRTypeCategory::Record: {
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
                auto type = rttr::get_type_from_guid(base->type_id);
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
    case ERTTRTypeCategory::Enum: {
        return (type_id == _enum_data.type_id || type_id == _enum_data.underlying_type_id);
    }
    default:
        SKR_UNREACHABLE_CODE()
        break;
    }
    return false;
}
} // namespace skr