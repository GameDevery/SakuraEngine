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
//
// script support primitive types:
//   void: in return
//   integer: int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t
//   floating: float, double
//   boolean: bool
//   string: skr::String, skr::StringView
//
// script support generic types:
//   skr::Array: accept script array
//   skr::Span: accept script array
//   skr::Vector: accept script array
//   skr::Map: accept script map
//   skr::Optional: make primitive/box type nullable without use pointer
//   skr::NotNull: make wrap type non-nullable without use reference
//   skr::FunctionRef: support callback from script for each() methods

namespace skr
{
enum class EScriptBinderError : uint64_t
{
    None = 0,

    // util error
    FailedInComponent          = 1 >> 0,
    UnsupportedType            = 1 >> 1,
    ValueTypeOfWrap            = 1 >> 2,
    PointerLevelGreaterThanOne = 1 >> 3,

    // field error
    ReferenceField                = 1 >> 4,
    PointerFieldForBoxOrPrimitive = 1 >> 5,

    // parameter error
    OutParamWithConst  = 1 >> 6,
    OutParamWithoutRef = 1 >> 7,

    // property error
    GetterSignatureError = 1 >> 8,
    SetterSignatureError = 1 >> 9,
    DuplicatedGetter     = 1 >> 10,
    DuplicatedSetter     = 1 >> 11,
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
    EScriptBinderError   error  = EScriptBinderError::None;
};
struct ScriptBinderStaticField {
    const RTTRType*            owner  = nullptr;
    ScriptBinderRoot           binder = {};
    const RTTRStaticFieldData* data   = nullptr;
    EScriptBinderError         error  = EScriptBinderError::None;
};

// nested binder, method & static method
struct ScriptBinderParam {
    ScriptBinderRoot   binder      = {};
    bool               pass_by_ref = false;
    bool               is_inout    = false; // TODO. use flag resolve
    bool               is_nullable = false;
    EScriptBinderError error       = EScriptBinderError::None;
};
struct ScriptBinderReturn {
    ScriptBinderRoot   binder      = {};
    bool               pass_by_ref = false;
    bool               is_nullable = false;
    EScriptBinderError error       = EScriptBinderError::None;
};
struct ScriptBinderMethod {
    struct Overload {
        const RTTRType*           owner         = nullptr;
        const RTTRMethodData*     data          = nullptr;
        ScriptBinderReturn        return_binder = {};
        Vector<ScriptBinderParam> params_binder = {};
        EScriptBinderError        error         = EScriptBinderError::None;
    };

    Vector<Overload>   overloads = {};
    EScriptBinderError error     = EScriptBinderError::None;
};
struct ScriptBinderStaticMethod {
    struct Overload {
        const RTTRType*             owner         = nullptr;
        const RTTRStaticMethodData* data          = nullptr;
        ScriptBinderReturn          return_binder = {};
        Vector<ScriptBinderParam>   params_binder = {};
        EScriptBinderError          error         = EScriptBinderError::None;
    };

    Vector<Overload>   overloads = {};
    EScriptBinderError error     = EScriptBinderError::None;
};

// nested binder, property
struct ScriptBinderProperty {
    ScriptBinderMethod setter = {};
    ScriptBinderMethod getter = {};
    EScriptBinderError error  = EScriptBinderError::None;
};
struct ScriptBinderStaticProperty {
    ScriptBinderStaticMethod setter = {};
    ScriptBinderStaticMethod getter = {};
    EScriptBinderError       error  = EScriptBinderError::None;
};

// nested binder, constructor
struct ScriptBinderCtor {
    const RTTRCtorData*       data          = nullptr;
    Vector<ScriptBinderParam> params_binder = {};
    EScriptBinderError        error         = EScriptBinderError::None;
};

// root binders
struct ScriptBinderPrimitive {
    uint32_t    size      = 0;
    uint32_t    alignment = 0;
    GUID        type_id   = {};
    DtorInvoker dtor      = nullptr;

    EScriptBinderError error = EScriptBinderError::None;
};
struct ScriptBinderBox {
    const RTTRType*                type;
    Map<String, ScriptBinderField> fields;

    EScriptBinderError error = EScriptBinderError::None;
};
struct ScriptBinderWrap {
    const RTTRType* type;

    bool                     is_script_newable = false;
    Vector<ScriptBinderCtor> ctors;

    Map<String, ScriptBinderField>          fields;
    Map<String, ScriptBinderStaticField>    static_fields;
    Map<String, ScriptBinderMethod>         methods;
    Map<String, ScriptBinderStaticMethod>   static_methods;
    Map<String, ScriptBinderProperty>       properties;
    Map<String, ScriptBinderStaticProperty> static_properties;

    EScriptBinderError error = EScriptBinderError::None;
};

// TODO. 为每个 binder 附加 Error 成员来存储错误，同时 nested make 都转为 void 返回，错误在导出完毕后统一进行处理
// TODO. In/Out flag
// TODO. Enum support
// TODO. Generic type support
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
    void _make_ctor(ScriptBinderCtor& out, const RTTRCtorData* ctor, const RTTRType* owner);
    void _make_method(ScriptBinderMethod::Overload& out, const RTTRMethodData* method, const RTTRType* owner);
    void _make_static_method(ScriptBinderStaticMethod::Overload& out, const RTTRStaticMethodData* method, const RTTRType* owner);
    void _make_prop_getter(ScriptBinderMethod& out, const RTTRMethodData* method, const RTTRType* owner);
    void _make_prop_setter(ScriptBinderMethod& out, const RTTRMethodData* method, const RTTRType* owner);
    void _make_static_prop_getter(ScriptBinderStaticMethod& out, const RTTRStaticMethodData* method, const RTTRType* owner);
    void _make_static_prop_setter(ScriptBinderStaticMethod& out, const RTTRStaticMethodData* method, const RTTRType* owner);
    void _make_field(ScriptBinderField& out, const RTTRFieldData* field, const RTTRType* owner);
    void _make_static_field(ScriptBinderStaticField& out, const RTTRStaticFieldData* field, const RTTRType* owner);
    void _make_param(ScriptBinderParam& out, StringView method_name, const RTTRParamData* param, const RTTRType* owner);
    void _make_return(ScriptBinderReturn& out, StringView method_name, TypeSignatureView signature, const RTTRType* owner);

    // checker
    EScriptBinderError _try_export_field(TypeSignatureView signature, ScriptBinderRoot& out_binder);

private:
    // cache
    Map<GUID, ScriptBinderRoot> _cached_root_binders;
};
} // namespace skr
