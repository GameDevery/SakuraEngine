#pragma once
#include <cstdint>
#include "SkrContainersDef/function_ref.hpp"
#include "SkrContainersDef/string.hpp"
#include "SkrContainersDef/string.hpp"
#include "SkrRTTR/enum_tools.hpp"
#include "SkrRTTR/export/export_data.hpp"
#include "SkrRTTR/export/export_builder.hpp"

// !!!! RTTR 不考虑动态类型建立(从脚本建立), 一切类型都是 CPP 静态注册的 loader !!!!
namespace skr
{
enum class ERTTRTypeCategory
{
    Invalid = 0, // uninitialized bad type

    Primitive,
    Record,
    Enum,
};
struct RTTRTypeCaster {
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
struct RTTRTypeEachConfig {
    // should each bases to find
    bool include_bases = true;
};
struct RTTRTypeFindConfig {
    // if setted, will filter by name
    Optional<StringView> name = {};

    // if setted, will filter by signature
    Optional<TypeSignatureView> signature              = {};
    ETypeSignatureCompareFlag   signature_compare_flag = ETypeSignatureCompareFlag::Strict;

    // should each bases to find
    bool include_bases = true;
};

struct SKR_CORE_API RTTRType final {
    // ctor & dtor
    RTTRType();
    ~RTTRType();

    // module
    void   set_module(String module);
    String module() const;

    // basic getter
    ERTTRTypeCategory  type_category() const;
    const skr::String& name() const;
    Vector<String>     name_space() const;
    GUID               type_id() const;
    size_t             size() const;
    size_t             alignment() const;
    void               each_name_space(FunctionRef<void(StringView)> each_func) const;

    // builders
    void build_primitive(FunctionRef<void(RTTRPrimitiveData* data)> func);
    void build_record(FunctionRef<void(RTTRRecordData* data)> func);
    void build_enum(FunctionRef<void(RTTREnumData* data)> func);

    // TODO. build optimize data
    //  1. fast base find table
    //  2. fast method/field table
    // void build_optimize_data();

    // TODO. check phase, used to check data conflict
    // void validate_export_data() const;

    // caster
    bool           based_on(GUID type_id) const;
    void*          cast_to_base(GUID type_id, void* p) const;
    RTTRTypeCaster caster_to_base(GUID type_id) const;

    // get dtor
    Optional<RTTRDtorData> dtor_data() const;

    // each method & field
    void each_bases(FunctionRef<void(const RTTRBaseData* base_data, const RTTRType* owner)> each_func, RTTRTypeEachConfig config = {}) const;
    void each_ctor(FunctionRef<void(const RTTRCtorData* ctor)> each_func, RTTRTypeEachConfig config = {}) const; // ignore [config.include_bases]x
    void each_method(FunctionRef<void(const RTTRMethodData* method, const RTTRType* owner)> each_func, RTTRTypeEachConfig config = {}) const;
    void each_field(FunctionRef<void(const RTTRFieldData* field, const RTTRType* owner)> each_func, RTTRTypeEachConfig config = {}) const;
    void each_static_method(FunctionRef<void(const RTTRStaticMethodData* method, const RTTRType* owner)> each_func, RTTRTypeEachConfig config = {}) const;
    void each_static_field(FunctionRef<void(const RTTRStaticFieldData* field, const RTTRType* owner)> each_func, RTTRTypeEachConfig config = {}) const;
    void each_extern_method(FunctionRef<void(const RTTRExternMethodData* method, const RTTRType* owner)> each_func, RTTRTypeEachConfig config = {}) const;

    // find method & field
    const RTTRCtorData*         find_ctor(RTTRTypeFindConfig config) const; // ignore [config.name/config.include_bases]
    const RTTRMethodData*       find_method(RTTRTypeFindConfig config) const;
    const RTTRFieldData*        find_field(RTTRTypeFindConfig config) const;
    const RTTRStaticMethodData* find_static_method(RTTRTypeFindConfig config) const;
    const RTTRStaticFieldData*  find_static_field(RTTRTypeFindConfig config) const;
    const RTTRExternMethodData* find_extern_method(RTTRTypeFindConfig config) const;

    // template find method & field
    // TODO. 使用包装器进行包装，辅助进行调用等行为
    template <typename Func>
    const RTTRCtorData* find_ctor_t(ETypeSignatureCompareFlag flag = ETypeSignatureCompareFlag::Strict) const;
    template <typename Func>
    const RTTRMethodData* find_method_t(StringView name, ETypeSignatureCompareFlag flag = ETypeSignatureCompareFlag::Strict, bool include_base = true) const;
    template <typename Field>
    const RTTRFieldData* find_field_t(StringView name, ETypeSignatureCompareFlag flag = ETypeSignatureCompareFlag::Strict, bool include_base = true) const;
    template <typename Func>
    const RTTRStaticMethodData* find_static_method_t(StringView name, ETypeSignatureCompareFlag flag = ETypeSignatureCompareFlag::Strict, bool include_base = true) const;
    template <typename Field>
    const RTTRStaticFieldData* find_static_field_t(StringView name, ETypeSignatureCompareFlag flag = ETypeSignatureCompareFlag::Strict, bool include_base = true) const;
    template <typename Func>
    const RTTRExternMethodData* find_extern_method_t(StringView name, ETypeSignatureCompareFlag flag = ETypeSignatureCompareFlag::Strict, bool include_base = true) const;

    // flag & attribute
    ERTTRRecordFlag   record_flag() const;
    ERTTREnumFlag     enum_flag() const;
    rttr::IAttribute* find_attribute(GUID attr_type_id) const;

private:
    // helpers
    bool _build_caster(RTTRTypeCaster& caster, const GUID& type_id) const;

private:
    ERTTRTypeCategory _type_category = ERTTRTypeCategory::Invalid;
    String            _module        = {};
    union
    {
        RTTRPrimitiveData _primitive_data;
        RTTRRecordData    _record_data;
        RTTREnumData      _enum_data;
    };
};
} // namespace skr

// type inline impl
namespace skr
{
// template find method & field
template <typename Func>
inline const RTTRCtorData* RTTRType::find_ctor_t(ETypeSignatureCompareFlag flag) const
{
    TypeSignatureTyped<Func> signature;
    return find_ctor(RTTRTypeFindConfig{
        .signature              = signature.view(),
        .signature_compare_flag = flag,
    });
}
template <typename Func>
inline const RTTRMethodData* RTTRType::find_method_t(StringView name, ETypeSignatureCompareFlag flag, bool include_base) const
{
    TypeSignatureTyped<Func> signature;
    return find_method(RTTRTypeFindConfig{
        .signature              = signature.view(),
        .signature_compare_flag = flag,
        .name                   = name,
        .include_bases          = include_base,
    });
}
template <typename Field>
inline const RTTRFieldData* RTTRType::find_field_t(StringView name, ETypeSignatureCompareFlag flag, bool include_base) const
{
    TypeSignatureTyped<Field> signature;
    return find_field(RTTRTypeFindConfig{
        .signature              = signature.view(),
        .signature_compare_flag = flag,
        .name                   = name,
        .include_bases          = include_base,
    });
}
template <typename Func>
inline const RTTRStaticMethodData* RTTRType::find_static_method_t(StringView name, ETypeSignatureCompareFlag flag, bool include_base) const
{
    TypeSignatureTyped<Func> signature;
    return find_static_method(RTTRTypeFindConfig{
        .signature              = signature.view(),
        .signature_compare_flag = flag,
        .name                   = name,
        .include_bases          = include_base,
    });
}
template <typename Field>
inline const RTTRStaticFieldData* RTTRType::find_static_field_t(StringView name, ETypeSignatureCompareFlag flag, bool include_base) const
{
    TypeSignatureTyped<Field> signature;
    return find_static_field(RTTRTypeFindConfig{
        .name                   = name,
        .signature              = signature.view(),
        .signature_compare_flag = flag,
        .include_bases          = include_base,
    });
}
template <typename Func>
inline const RTTRExternMethodData* RTTRType::find_extern_method_t(StringView name, ETypeSignatureCompareFlag flag, bool include_base) const
{
    TypeSignatureTyped<Func> signature;
    return find_extern_method(RTTRTypeFindConfig{
        .name                   = name,
        .signature              = signature.view(),
        .signature_compare_flag = flag,
        .include_bases          = include_base,
    });
}
} // namespace skr