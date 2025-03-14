#pragma once
#include "SkrContainersDef/map.hpp"
#include "SkrContainersDef/optional.hpp"
#include "SkrContainersDef/vector.hpp"
#include "SkrRTTR/rttr_traits.hpp"
#include "SkrRTTR/type_signature.hpp"
#include "SkrRTTR/enum_tools.hpp"
#include "SkrRTTR/export/attribute.hpp"
#include "SkrRTTR/export/dynamic_stack.hpp"
#ifndef __meta__
    #include "SkrCore/SkrRTTR/export/export_data.generated.h"
#endif

// utils
namespace skr
{
// basic enums
// clang-format off
sreflect_enum_class(guid = "26ff0860-5d65-4ece-896e-225b3c083ecb")
ERTTRAccessLevel : uint8_t
// clang-format on
{
    Public,
    Protected,
    Private
};

// flags
// clang-format off
sreflect_enum_class(guid = "3874222c-79a4-4868-b146-d90c925914e0")
ERTTRParamFlag : uint32_t
// clang-format on
{
    None = 0,      // default
    In   = 1 << 0, // is input native pointer/reference
    Out  = 1 << 1, // is output native pointer/reference
};
// clang-format off
sreflect_enum_class(guid = "cf941db5-afa5-476e-9890-af836a373e73")
ERTTRFunctionFlag : uint32_t
// clang-format on
{
    None          = 0,      // default
    ScriptVisible = 1 << 0, // can script visit this function
};
// clang-format off
sreflect_enum_class(guid = "42d77f46-f3e0-492d-b67f-f8445eff2885")
ERTTRMethodFlag : uint32_t
{
    None          = 0,      // default
    ScriptVisible = 1 << 0, // can script visit this method
};
// clang-format off
sreflect_enum_class(guid = "52c78c63-9973-49f4-9118-a8b59b9ceb9e")
ERTTRStaticMethodFlag : uint32_t
// clang-format on
{
    None          = 0,      // default
    ScriptVisible = 1 << 0, // can script visit this static method
};
// clang-format off
sreflect_enum_class(guid = "f7199493-29f5-4235-86c1-13ec7541917b")
ERTTRExternMethodFlag : uint32_t
// clang-format on
{
    None          = 0,      // default
    ScriptVisible = 1 << 0, // can script visit this static method
};
// clang-format off
sreflect_enum_class(guid = "2f8c20d1-34b2-4ebe-bfd6-258a9e4c0c9e")
ERTTRFieldFlag : uint32_t
// clang-format on
{
    None          = 0,      // default
    ScriptVisible = 1 << 0, // can script visit this field
    ScriptWrap    = 1 << 1, // force export field use wrap mode
};
// clang-format off
sreflect_enum_class(guid = "396e9de1-ce51-4a65-8e47-09525e91207f")
ERTTRStaticFieldFlag : uint32_t
// clang-format on
{
    None          = 0,      // default
    ScriptVisible = 1 << 0, // can script visit this static field
    ScriptWrap    = 1 << 1, // force export field use wrap mode
};
// clang-format off
sreflect_enum_class(guid = "1f2aa88d-4d2f-47c0-8c97-b03cf574d673")
ERTTRCtorFlag : uint32_t
// clang-format on
{
    None          = 0,
    ScriptVisible = 1 << 0, // can script visit this ctor
};
// clang-format off
sreflect_enum_class(guid = "a1497c67-9865-44cb-b81d-08c4e9548ae9")
ERTTRRecordFlag : uint32_t
// clang-format on
{
    None          = 0,      // default
    ScriptVisible = 1 << 0, // can script visit this record
    ScriptNewable = 1 << 1, // can script new this record
    ScriptBox     = 1 << 2, // export to script use box mode
};
// clang-format off
sreflect_enum_class(guid = "a2d04427-aa0a-43b4-9975-bc0e6b92120e")
ERTTREnumItemFlag : uint32_t
// clang-format on
{
    None          = 0,      // default
    ScriptVisible = 1 << 0, // can script visit this enum item
};
// clang-format off
sreflect_enum_class(guid = "e76f7ad6-303d-4704-9065-827a7f6f270d")
ERTTREnumFlag : uint32_t
// clang-format on
{
    None          = 0,      // default
    ScriptVisible = 1 << 0, // can script visit this enum
    Flag          = 1 << 1, // is flag enum
};

// util data
struct RTTRParamData {
    using MakeDefaultFunc = void (*)(void*);

    // signature
    String        name = {};
    TypeSignature type = {};
    // TODO. make default 需要处理 xvalue 的 case，以及判断是否是 InitListExpr
    //       同时，需要通过是否是 ExprWithCleanups 来判断是否是 xvalue 类型的构造
    MakeDefaultFunc make_default = nullptr;

    // TODO. flag & Attribute
    ERTTRParamFlag               flag = ERTTRParamFlag::None;
    Map<GUID, attr::IAttribute*> attributes;

    template <typename Arg>
    inline static RTTRParamData Make()
    {
        RTTRParamData result;
        result.type = type_signature_of<Arg>();
        return result;
    }
};

// help functions
template <typename Data>
inline bool export_function_signature_equal(const Data& data, TypeSignature signature, ETypeSignatureCompareFlag flag)
{
    SKR_ASSERT(signature.view().is_complete());
    SKR_ASSERT(signature.view().is_function());

    // read signature data
    uint32_t param_count;
    auto     view = signature.view().read_function_signature(param_count);
    if (param_count != data.param_data.size()) { return false; }

    // compare return type
    auto ret_view = view.jump_next_type_or_data();
    if (!data.ret_type.view().equal(ret_view, flag)) { return false; }

    // compare param type
    for (uint32_t i = 0; i < param_count; ++i)
    {
        auto param_view = view.jump_next_type_or_data();
        if (!data.param_data[i].type.view().equal(param_view, flag)) { return false; }
    }

    return true;
}
} // namespace skr

// functions and methods
namespace skr
{
struct RTTRFunctionData {
    // signature
    String                name       = {};
    Vector<String>        name_space = {};
    TypeSignature         ret_type   = {};
    Vector<RTTRParamData> param_data = {};

    // [Provided by export Backend]
    void*                   native_invoke        = nullptr;
    FuncInvokerDynamicStack dynamic_stack_invoke = nullptr;

    // flag & attributes
    ERTTRFunctionFlag            flag       = ERTTRFunctionFlag::None;
    Map<GUID, attr::IAttribute*> attributes = {};

    inline ~RTTRFunctionData()
    {
        // delete attributes
        for (const auto& it : attributes)
        {
            SkrDelete(it.value);
        }
    }

    template <typename Ret, typename... Args>
    inline void fill_signature(Ret (*)(Args...))
    {
        ret_type   = type_signature_of<Ret>();
        param_data = { RTTRParamData::Make<Args>()... };
    }

    inline bool signature_equal(TypeSignature signature, ETypeSignatureCompareFlag flag) const
    {
        return export_function_signature_equal(*this, signature, flag);
    }
};
struct RTTRMethodData {
    // signature
    String                name         = {};
    TypeSignature         ret_type     = {};
    Vector<RTTRParamData> param_data   = {};
    bool                  is_const     = false;
    ERTTRAccessLevel      access_level = ERTTRAccessLevel::Public;

    // [Provided by export Backend]
    void*                     native_invoke        = nullptr;
    MethodInvokerDynamicStack dynamic_stack_invoke = nullptr;

    // flag & attributes
    ERTTRMethodFlag              flag       = ERTTRMethodFlag::None;
    Map<GUID, attr::IAttribute*> attributes = {};

    inline ~RTTRMethodData()
    {
        // delete attributes
        for (const auto& it : attributes)
        {
            SkrDelete(it.value);
        }
    }

    template <class T, typename Ret, typename... Args>
    inline void fill_signature(Ret (T::*)(Args...))
    {
        ret_type   = type_signature_of<Ret>();
        param_data = { RTTRParamData::Make<Args>()... };
        is_const   = false;
    }
    template <class T, typename Ret, typename... Args>
    inline void fill_signature(Ret (T::*)(Args...) const)
    {
        ret_type   = type_signature_of<Ret>();
        param_data = { RTTRParamData::Make<Args>()... };
        is_const   = true;
    }

    inline bool signature_equal(TypeSignature signature, ETypeSignatureCompareFlag flag) const
    {
        return export_function_signature_equal(*this, signature, flag);
    }
};
struct RTTRStaticMethodData {
    // signature
    String                name         = {};
    TypeSignature         ret_type     = {};
    Vector<RTTRParamData> param_data   = {};
    ERTTRAccessLevel      access_level = ERTTRAccessLevel::Public;

    // [Provided by export Backend]
    void*                   native_invoke        = nullptr;
    FuncInvokerDynamicStack dynamic_stack_invoke = nullptr;

    // flag & attributes
    ERTTRStaticMethodFlag        flag       = ERTTRStaticMethodFlag::None;
    Map<GUID, attr::IAttribute*> attributes = {};

    inline ~RTTRStaticMethodData()
    {
        // delete attributes
        for (const auto& it : attributes)
        {
            SkrDelete(it.value);
        }
    }

    template <typename Ret, typename... Args>
    inline void fill_signature(Ret (*)(Args...))
    {
        ret_type   = type_signature_of<Ret>();
        param_data = { RTTRParamData::Make<Args>()... };
    }

    inline bool signature_equal(TypeSignature signature, ETypeSignatureCompareFlag flag) const
    {
        return export_function_signature_equal(*this, signature, flag);
    }
};
struct RTTRExternMethodData {
    // signature
    String                name         = {};
    TypeSignature         ret_type     = {};
    Vector<RTTRParamData> param_data   = {};
    ERTTRAccessLevel      access_level = ERTTRAccessLevel::Public;

    // [Provided by export Backend]
    void*                   native_invoke        = nullptr;
    FuncInvokerDynamicStack dynamic_stack_invoke = nullptr;

    // flag & attributes
    ERTTRExternMethodFlag        flag       = ERTTRExternMethodFlag::None;
    Map<GUID, attr::IAttribute*> attributes = {};

    inline ~RTTRExternMethodData()
    {
        // delete attributes
        for (const auto& it : attributes)
        {
            SkrDelete(it.value);
        }
    }

    template <typename Ret, typename... Args>
    inline void fill_signature(Ret (*)(Args...))
    {
        ret_type   = type_signature_of<Ret>();
        param_data = { RTTRParamData::Make<Args>()... };
    }

    inline bool signature_equal(TypeSignature signature, ETypeSignatureCompareFlag flag) const
    {
        return export_function_signature_equal(*this, signature, flag);
    }
};
struct RTTRCtorData {
    // signature
    Vector<RTTRParamData> param_data   = {};
    ERTTRAccessLevel      access_level = ERTTRAccessLevel::Public;

    // [Provided by export Backend]
    void*                     native_invoke        = nullptr;
    MethodInvokerDynamicStack dynamic_stack_invoke = nullptr;

    // flag & attributes
    ERTTRCtorFlag                flag       = ERTTRCtorFlag::None;
    Map<GUID, attr::IAttribute*> attributes = {};

    inline ~RTTRCtorData()
    {
        // delete attributes
        for (const auto& it : attributes)
        {
            SkrDelete(it.value);
        }
    }

    template <typename... Args>
    inline void fill_signature()
    {
        param_data = { RTTRParamData::Make<Args>()... };
    }

    inline bool signature_equal(TypeSignature signature, ETypeSignatureCompareFlag flag) const
    {
        SKR_ASSERT(signature.view().is_complete());
        SKR_ASSERT(signature.view().is_function());

        // read signature data
        uint32_t param_count;
        auto     view = signature.view().read_function_signature(param_count);
        if (param_count != param_data.size()) { return false; }

        // compare return type
        auto                     ret_view = view.jump_next_type_or_data();
        TypeSignatureTyped<void> ret_type;
        if (!ret_type.view().equal(ret_view, flag)) { return false; }

        // compare param type
        for (uint32_t i = 0; i < param_count; ++i)
        {
            auto param_view = view.jump_next_type_or_data();
            if (!param_data[i].type.view().equal(param_view, flag)) { return false; }
        }

        return true;
    }
};
} // namespace skr

// fields
namespace skr
{
struct RTTRFieldData {
    using GetAddressFunc = void* (*)(void*);

    // signature
    String           name         = {};
    TypeSignature    type         = {};
    GetAddressFunc   get_address  = nullptr;
    ERTTRAccessLevel access_level = ERTTRAccessLevel::Public;

    // flag & attributes
    ERTTRFieldFlag               flag       = ERTTRFieldFlag::None;
    Map<GUID, attr::IAttribute*> attributes = {};

    inline ~RTTRFieldData()
    {
        // delete attributes
        for (const auto& it : attributes)
        {
            SkrDelete(it.value);
        }
    }

    template <auto field, class T, typename Field>
    inline void fill_signature(Field T::*)
    {
        type        = type_signature_of<Field>();
        get_address = +[](void* p) -> void* {
            return &(reinterpret_cast<T*>(p)->*field);
        };
    }
};
struct RTTRStaticFieldData {
    // signature
    String           name         = {};
    TypeSignature    type         = {};
    void*            address      = nullptr;
    ERTTRAccessLevel access_level = ERTTRAccessLevel::Public;

    // flag & attributes
    ERTTRStaticFieldFlag         flag       = ERTTRStaticFieldFlag::None;
    Map<GUID, attr::IAttribute*> attributes = {};

    inline ~RTTRStaticFieldData()
    {
        // delete attributes
        for (const auto& it : attributes)
        {
            SkrDelete(it.value);
        }
    }

    template <typename T>
    inline void fill_signature(T* p_field)
    {
        type = type_signature_of<T>();
    }
};
} // namespace skr

// record
namespace skr
{
struct RTTRBaseData {
    using CastFunc = void* (*)(void*);

    GUID     type_id;
    CastFunc cast_to_base;

    template <typename T, typename Base>
    inline static RTTRBaseData Make()
    {
        return {
            RTTRTraits<Base>::get_guid(),
            +[](void* p) -> void* {
                return static_cast<Base*>(reinterpret_cast<T*>(p));
            }
        };
    }
    template <typename T, typename Base>
    inline static RTTRBaseData* New()
    {
        return SkrNew<RTTRBaseData>(
            RTTRTraits<Base>::get_guid(),
            +[](void* p) -> void* {
                return static_cast<Base*>(reinterpret_cast<T*>(p));
            }
        );
    }
};
using DtorInvoker = void (*)(void*);
struct RTTRDtorData {
    ERTTRAccessLevel access_level;

    // [Provided by export Backend]
    DtorInvoker native_invoke;
};
struct RTTRRecordData {
    // basic
    String         name       = {};
    Vector<String> name_space = {};
    GUID           type_id    = {};
    size_t         size       = 0;
    size_t         alignment  = 0;

    // bases
    Vector<RTTRBaseData*> bases_data = {};

    // ctor & dtor
    Vector<RTTRCtorData*> ctor_data = {};
    RTTRDtorData          dtor_data = {};

    // method & fields
    Vector<RTTRMethodData*> methods = {};
    Vector<RTTRFieldData*>  fields  = {};

    // static method & static fields
    Vector<RTTRStaticMethodData*> static_methods = {};
    Vector<RTTRStaticFieldData*>  static_fields  = {};

    // extern method
    Vector<RTTRExternMethodData*> extern_methods = {};

    // flag & attributes
    ERTTRRecordFlag              flag       = ERTTRRecordFlag::None;
    Map<GUID, attr::IAttribute*> attributes = {};

    inline ~RTTRRecordData()
    {
        // delete bases
        for (auto base : bases_data)
        {
            SkrDelete(base);
        }

        // delete ctors
        for (auto ctor : ctor_data)
        {
            SkrDelete(ctor);
        }

        // delete methods & fields
        for (auto method : methods)
        {
            SkrDelete(method);
        }
        for (auto field : fields)
        {
            SkrDelete(field);
        }

        // delete static methods & static fields
        for (auto method : static_methods)
        {
            SkrDelete(method);
        }
        for (auto field : static_fields)
        {
            SkrDelete(field);
        }

        // delete extern methods
        for (auto method : extern_methods)
        {
            SkrDelete(method);
        }

        // delete attributes
        for (const auto& it : attributes)
        {
            SkrDelete(it.value);
        }
    }
};
} // namespace skr

// enum
namespace skr
{
struct RTTREnumItemData {
    String    name  = {};
    EnumValue value = {};

    // flag & attributes
    ERTTREnumItemFlag            flag       = ERTTREnumItemFlag::None;
    Map<GUID, attr::IAttribute*> attributes = {};

    inline ~RTTREnumItemData()
    {
        // delete attributes
        for (const auto& it : attributes)
        {
            SkrDelete(it.value);
        }
    }
};
struct RTTREnumData {
    // basic
    String         name       = {};
    Vector<String> name_space = {};
    GUID           type_id    = {};
    size_t         size       = 0;
    size_t         alignment  = 0;

    // underlying type
    GUID underlying_type_id = {};

    // items
    Vector<RTTREnumItemData> items = {};

    // extern method
    Vector<RTTRExternMethodData*> extern_methods = {};

    // flag & attributes
    ERTTREnumFlag                flag       = ERTTREnumFlag::None;
    Map<GUID, attr::IAttribute*> attributes = {};

    inline ~RTTREnumData()
    {
        // delete attributes
        for (const auto& it : attributes)
        {
            SkrDelete(it.value);
        }
    }
};
} // namespace skr

// primitive data
namespace skr
{
struct RTTRPrimitiveData {
    // basic
    String name;
    GUID   type_id;
    size_t size;
    size_t alignment;

    // extern method
    Vector<RTTRExternMethodData*> extern_methods;
};
} // namespace skr
