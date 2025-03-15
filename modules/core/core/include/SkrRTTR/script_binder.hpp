#pragma once
#include <SkrRTTR/type.hpp>

// script export behaviour map
// parameter:
// |            |  primitive |    box    |   wrap   |
// |     T      |      T     |     T     |    -     |
// |     T*     |   inout T? |  inout T? |    T?    |
// |  const T*  |      T?    |     T?    |    T?    |
// |     T&     |   inout T  |  inout T  |    T     |
// |  const T&  |      T     |     T     |    T     |
//
// return:
// |            |  primitive |    box    |   wrap   |
// |     T      |      T     |     T     |    -     |
// |     T*     |      T?    |     T?    |    T?    |
// |  const T*  |      T?    |     T?    |    T?    |
// |     T&     |      T     |     T     |    T     |
// |  const T&  |      T     |     T     |    T     |
//
// field:
// |            |  primitive |    box    |   wrap   |
// |     T      |      T     |     T     |    -     |
// |     T*     |      -     |     -     |    T?    |
// |  const T*  |      -     |     -     |    T?    |
// |     T&     |      -     |     -     |    -     |
// |  const T&  |      -     |     -     |    -     |

namespace skr
{
// failed id
enum class EScriptBindFailed
{
    None,
    Unknown,
    ExportWrapByValue,
    ExportPointerFieldOfPrimitiveOrBox,
    ExportReferenceField,
    ExportPointerLevelGreaterThanOne,
};

// root binder, used for nested binder
struct ScriptBinderPrimitive;
struct ScriptBinderBox;
struct ScriptBinderWrap;
struct ScriptBinderRoot {
    enum class EKind
    {
        None,
        Primitive,
        Box,
        Wrap,
    };

    // ctor
    inline ScriptBinderRoot() = default;
    inline ScriptBinderRoot(ScriptBinderPrimitive* primitive)
        : _kind(EKind::Primitive)
        , _binder(primitive)
    {
    }
    inline ScriptBinderRoot(ScriptBinderBox* box)
        : _kind(EKind::Box)
        , _binder(box)
    {
    }
    inline ScriptBinderRoot(ScriptBinderWrap* wrap)
        : _kind(EKind::Wrap)
        , _binder(wrap)
    {
    }

    // copy & move
    inline ScriptBinderRoot(const ScriptBinderRoot& other) = default;
    inline ScriptBinderRoot(ScriptBinderRoot&& other)      = default;

    // assign & move assign
    inline ScriptBinderRoot& operator=(const ScriptBinderRoot& other) = default;
    inline ScriptBinderRoot& operator=(ScriptBinderRoot&& other)      = default;

    // kind getter
    inline EKind kind() const { return _kind; }
    inline bool  is_empty() const { return _kind == EKind::None; }
    inline bool  is_primitive() const { return _kind == EKind::Primitive; }
    inline bool  is_box() const { return _kind == EKind::Box; }
    inline bool  is_wrap() const { return _kind == EKind::Wrap; }

    // binder getter
    inline ScriptBinderPrimitive* primitive() const
    {
        SKR_ASSERT(_kind == EKind::Primitive);
        return static_cast<ScriptBinderPrimitive*>(_binder);
    }
    inline ScriptBinderBox* box() const
    {
        SKR_ASSERT(_kind == EKind::Box);
        return static_cast<ScriptBinderBox*>(_binder);
    }
    inline ScriptBinderWrap* wrap() const
    {
        SKR_ASSERT(_kind == EKind::Wrap);
        return static_cast<ScriptBinderWrap*>(_binder);
    }

    // ops
    inline void reset()
    {
        _kind   = EKind::None;
        _binder = nullptr;
    }

private:
    EKind _kind   = EKind::None;
    void* _binder = nullptr;
};

// nested binder, field & static field
struct ScriptBinderField {
    const RTTRType*      owner  = nullptr;
    ScriptBinderRoot     binder = {};
    const RTTRFieldData* data   = nullptr;
};
struct ScriptBinderStaticField {
    const RTTRType*            owner  = nullptr;
    ScriptBinderRoot           binder = {};
    const RTTRStaticFieldData* data   = nullptr;
};

// nested binder, method & static method
struct ScriptBinderParam {
    ScriptBinderRoot binder      = {};
    bool             pass_by_ref = false;
    bool             is_inout    = false; // TODO. use flag resolve
    bool             is_nullable = false;
};
struct ScriptBinderReturn {
    ScriptBinderRoot binder      = {};
    bool             pass_by_ref = false;
    bool             is_nullable = false;
};
struct ScriptBinderMethod {
    struct Overload {
        const RTTRType*           owner         = nullptr;
        const RTTRMethodData*     data          = nullptr;
        ScriptBinderReturn        return_binder = {};
        Vector<ScriptBinderParam> params_binder = {};
    };

    Vector<Overload> overloads = {};
};
struct ScriptBinderStaticMethod {
    struct Overload {
        const RTTRType*             owner         = nullptr;
        const RTTRStaticMethodData* data          = nullptr;
        ScriptBinderReturn          return_binder = {};
        Vector<ScriptBinderParam>   params_binder = {};
    };

    Vector<Overload> overloads = {};
};

// nested binder, constructor
struct ScriptBinderCtor {
    const RTTRCtorData*       data          = nullptr;
    Vector<ScriptBinderParam> params_binder = {};
};

// root binders
struct ScriptBinderPrimitive {
    uint32_t    size      = 0;
    uint32_t    alignment = 0;
    GUID        type_id   = {};
    DtorInvoker dtor      = nullptr;
};
struct ScriptBinderBox {
    const RTTRType*                type;
    Map<String, ScriptBinderField> fields;
};
struct ScriptBinderWrap {
    const RTTRType* type;

    bool                     is_script_newable = false;
    Vector<ScriptBinderCtor> ctors;

    Map<String, ScriptBinderField>        fields;
    Map<String, ScriptBinderStaticField>  static_fields;
    Map<String, ScriptBinderMethod>       methods;
    Map<String, ScriptBinderStaticMethod> static_methods;
};

struct SKR_CORE_API ScriptBinderManager {
    // ctor & dtor
    ScriptBinderManager();
    ~ScriptBinderManager();

    // get binder
    ScriptBinderRoot get_or_build(GUID type_id);

private:
    // make root binder
    ScriptBinderPrimitive* _make_primitive(GUID type_id);
    ScriptBinderBox*       _make_box(const RTTRType* type);
    ScriptBinderWrap*      _make_wrap(const RTTRType* type);

    // make nested binder
    bool _make_ctor(ScriptBinderCtor& out, const RTTRCtorData* ctor, const RTTRType* owner);
    bool _make_method(ScriptBinderMethod::Overload& out, const RTTRMethodData* method, const RTTRType* owner);
    bool _make_static_method(ScriptBinderStaticMethod::Overload& out, const RTTRStaticMethodData* method, const RTTRType* owner);
    bool _make_field(ScriptBinderField& out, const RTTRFieldData* field, const RTTRType* owner);
    bool _make_static_field(ScriptBinderStaticField& out, const RTTRStaticFieldData* field, const RTTRType* owner);
    bool _make_param(ScriptBinderParam& out, StringView method_name, const RTTRParamData* param, const RTTRType* owner);
    bool _make_return(ScriptBinderReturn& out, StringView method_name, TypeSignatureView signature, const RTTRType* owner);

    // checker
    EScriptBindFailed _try_export_field(TypeSignatureView signature, ScriptBinderRoot& out_binder);

    // error helper
    StringView _err_string(EScriptBindFailed err);
    void       _err_field(StringView field_name, StringView owner_name, EScriptBindFailed err);
    void       _err_param(StringView param_name, StringView method_name, EScriptBindFailed err);
    void       _err_return(StringView method_name, EScriptBindFailed err);

private:
    // cache
    Map<GUID, ScriptBinderRoot> _cached_root_binders;
};
} // namespace skr
