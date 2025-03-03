#pragma once
#include <cstdint>
#include "SkrContainersDef/function_ref.hpp"
#include "SkrContainersDef/string.hpp"
#include "SkrContainersDef/string.hpp"
#include "SkrRTTR/enum_tools.hpp"
#include "SkrRTTR/export/export_data.hpp"
#include "SkrRTTR/export/export_builder.hpp"

// !!!! RTTR 不考虑动态类型建立(从脚本建立), 一切类型都是 CPP 静态注册的 loader !!!!
namespace skr::rttr
{
enum class ETypeCategory
{
    Invalid = 0, // uninitialized bad type

    Primitive,
    Record,
    Enum,
};
struct TypeCaster {
    using CastFunc = void* (*)(void*);

    inline void* cast(void* p)
    {
        if (!is_valid)
        {
            return nullptr;
        }

        for (auto func : cast_funcs)
        {
            p = func(p);
        }
        return p;
    }

    bool                      is_valid   = true;
    InlineVector<CastFunc, 8> cast_funcs = {};
};
struct TypeEachConfig {
    // should each bases to find
    bool include_bases = true;
};
struct TypeFindConfig {
    // if setted, will filter by name
    Optional<StringView> name = {};

    // if setted, will filter by signature
    Optional<TypeSignatureView> signature = {};
    ETypeSignatureCompareFlag   signature_compare_flag = ETypeSignatureCompareFlag::Strict;

    // should each bases to find
    bool include_bases = true;
};


struct SKR_CORE_API Type final {
    // ctor & dtor
    Type();
    ~Type();

    // basic getter
    ETypeCategory      type_category() const;
    const skr::String& name() const;
    Vector<String>     name_space() const;
    GUID               type_id() const;
    size_t             size() const;
    size_t             alignment() const;
    void               each_name_space(FunctionRef<void(StringView)> each_func) const;

    // builders
    void build_primitive(FunctionRef<void(PrimitiveData* data)> func);
    void build_record(FunctionRef<void(RecordData* data)> func);
    void build_enum(FunctionRef<void(EnumData* data)> func);

    // TODO. build optimize data
    //  1. fast base find table
    //  2. fast method/field table
    // void build_optimize_data();

    // TODO. check phase, used to check data conflict
    // void validate_export_data() const;

    // caster
    void*      cast_to(GUID type_id, void* p) const;
    TypeCaster caster(GUID type_id) const;

    // get dtor
    Optional<DtorData> dtor_data() const;

    // each method & field
    void each_bases(FunctionRef<void(const BaseData* base_data, const Type* owner)> each_func, TypeEachConfig config = {}) const;
    void each_ctor(FunctionRef<void(const CtorData* ctor)> each_func, TypeEachConfig config = {}) const; // ignore [config.include_bases]x
    void each_method(FunctionRef<void(const MethodData* method, const Type* owner)> each_func, TypeEachConfig config = {}) const;
    void each_field(FunctionRef<void(const FieldData* field, const Type* owner)> each_func, TypeEachConfig config = {}) const;
    void each_static_method(FunctionRef<void(const StaticMethodData* method, const Type* owner)> each_func, TypeEachConfig config = {}) const;
    void each_static_field(FunctionRef<void(const StaticFieldData* field, const Type* owner)> each_func, TypeEachConfig config = {}) const;
    void each_extern_method(FunctionRef<void(const ExternMethodData* method, const Type* owner)> each_func, TypeEachConfig config = {}) const;

    // find method & field
    const CtorData*         find_ctor(TypeFindConfig config) const; // ignore [config.name/config.include_bases]
    const MethodData*       find_method(TypeFindConfig config) const;
    const FieldData*        find_field(TypeFindConfig config) const;
    const StaticMethodData* find_static_method(TypeFindConfig config) const;
    const StaticFieldData*  find_static_field(TypeFindConfig config) const;
    const ExternMethodData* find_extern_method(TypeFindConfig config) const;

    // template find method & field
    template <typename Func>
    const CtorData* find_ctor_t(ETypeSignatureCompareFlag flag = ETypeSignatureCompareFlag::Strict) const;
    template <typename Func>
    const MethodData* find_method_t(StringView name, ETypeSignatureCompareFlag flag = ETypeSignatureCompareFlag::Strict, bool include_base = true) const;
    template <typename Field>
    const FieldData* find_field_t(StringView name, ETypeSignatureCompareFlag flag = ETypeSignatureCompareFlag::Strict, bool include_base = true) const;
    template <typename Func>
    const StaticMethodData* find_static_method_t(StringView name, ETypeSignatureCompareFlag flag = ETypeSignatureCompareFlag::Strict, bool include_base = true) const;
    template <typename Field>
    const StaticFieldData* find_static_field_t(StringView name, ETypeSignatureCompareFlag flag = ETypeSignatureCompareFlag::Strict, bool include_base = true) const;
    template <typename Func>
    const ExternMethodData* find_extern_method_t(StringView name, ETypeSignatureCompareFlag flag = ETypeSignatureCompareFlag::Strict, bool include_base = true) const;

    // flag & attribute
    ERecordFlag record_flag() const;
    EEnumFlag   enum_flag() const;
    IAttribute* find_attribute(GUID attr_type_id) const;

private:
    // helpers
    bool _build_caster(TypeCaster& caster, const GUID& type_id) const;

private:
    ETypeCategory _type_category = ETypeCategory::Invalid;
    union
    {
        PrimitiveData _primitive_data;
        RecordData    _record_data;
        EnumData      _enum_data;
    };
};
} // namespace skr::rttr

// type inline impl
namespace skr::rttr
{
// template find method & field
template <typename Func>
inline const CtorData* Type::find_ctor_t(ETypeSignatureCompareFlag flag) const
{
    TypeSignatureTyped<Func> signature;
    return find_ctor(TypeFindConfig {
        .signature = signature.view(),
        .signature_compare_flag = flag,
    });
}
template <typename Func>
inline const MethodData* Type::find_method_t(StringView name, ETypeSignatureCompareFlag flag, bool include_base) const
{
    TypeSignatureTyped<Func> signature;
    return find_method(TypeFindConfig {
        .signature = signature.view(),
        .signature_compare_flag = flag,
        .name = name,
        .include_bases = include_base,
    });
}
template <typename Field>
inline const FieldData* Type::find_field_t(StringView name, ETypeSignatureCompareFlag flag, bool include_base) const
{
    TypeSignatureTyped<Field> signature;
    return find_field(TypeFindConfig {
        .signature = signature.view(),
        .signature_compare_flag = flag,
        .name = name,
        .include_bases = include_base,
    });
}
template <typename Func>
inline const StaticMethodData* Type::find_static_method_t(StringView name, ETypeSignatureCompareFlag flag, bool include_base) const
{
    TypeSignatureTyped<Func> signature;
    return find_static_method(TypeFindConfig {
        .signature = signature.view(),
        .signature_compare_flag = flag,
        .name = name,
        .include_bases = include_base,
    });
}
template <typename Field>
inline const StaticFieldData* Type::find_static_field_t(StringView name, ETypeSignatureCompareFlag flag, bool include_base) const
{
    TypeSignatureTyped<Field> signature;
    return find_static_field(TypeFindConfig {
        .signature = signature.view(),
        .signature_compare_flag = flag,
        .name = name,
        .include_bases = include_base,
    });
}
template <typename Func>
inline const ExternMethodData* Type::find_extern_method_t(StringView name, ETypeSignatureCompareFlag flag, bool include_base) const
{
    TypeSignatureTyped<Func> signature;
    return find_extern_method(TypeFindConfig {
        .signature = signature.view(),
        .signature_compare_flag = flag,
        .name = name,
        .include_bases = include_base,
    });
}
} // namespace skr::rttr