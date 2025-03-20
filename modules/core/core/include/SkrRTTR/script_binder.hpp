#pragma once
#include <SkrRTTR/type.hpp>
#include <SkrCore/log.hpp>

// script export concept
//   - primitive: primitive type, always include [number, boolean, string, real]
//   - mapping: map type structure into a script object, methods is not supported
//   - value: support full record feature, script object will hold an pointer of native object,
//            but not have lifetime control, lifetime always follow it's creator
//   - object: support full record feature, script object will hold an pointer of native object,
//             and have lifetime control, witch implemented by ScriptbleObject,
//             ONLY classes that inherit ScriptbleObject can be export as object
//
// export behaviour:
//   parameter(call native):
//   |            |  primitive |  enum   |  mapping  |  value  |  object  |
//   |     T      |   (copy)T  | (copy)T |  (copy)T  | (copy)T |    -     |
//   |     T*     |      -     |    -    |     -     |    -    |(by ref)T?|
//   |  const T*  |      -     |    -    |     -     |    -    |(by ref)T?|
//   |     T&     |   (copy)T  | (copy)T |  (copy)T  |(by ref)T|    -     | Note. by default, will have inout flag
//   |  const T&  |   (copy)T  | (copy)T |  (copy)T  |(by ref)T|    -     |
//
//   parameter(call script):
//   |            |  primitive |  enum   |  mapping  |  value  |  object  |
//   |     T      |   (copy)T  | (copy)T |  (copy)T  |(by ref)T|    -     |
//   |     T*     |      -     |    -    |     -     |    -    |(by ref)T?|
//   |  const T*  |      -     |    -    |     -     |    -    |(by ref)T?|
//   |     T&     |   (copy)T  | (copy)T |  (copy)T  |(by ref)T|    -     |
//   |  const T&  |   (copy)T  | (copy)T |  (copy)T  |(by ref)T|    -     |
//
//   return(call native):
//   |            |  primitive |  enum   |  mapping  |  value  |  object  |
//   |     T      |   (copy)T  | (copy)T |  (copy)T  | (copy)T |    -     |
//   |     T*     |      -     |    -    |     -     |    -    |(by ref)T?|
//   |  const T*  |      -     |    -    |     -     |    -    |(by ref)T?|
//   |     T&     |   (copy)T  | (copy)T |  (copy)T  | (copy)T |    -     |
//   |  const T&  |   (copy)T  | (copy)T |  (copy)T  | (copy)T |    -     |
//
//   return(call script):
//   |            |  primitive |  enum   |  mapping  |  value  |  object  |
//   |     T      |   (copy)T  | (copy)T |  (copy)T  | (copy)T |    -     |
//   |     T*     |      -     |    -    |     -     |    -    |(by ref)T?|
//   |  const T*  |      -     |    -    |     -     |    -    |(by ref)T?|
//   |     T&     |      -     |    -    |     -     |    -    |    -     |
//   |  const T&  |      -     |    -    |     -     |    -    |    -     |
//
//   field(native -> script):
//   |            |  primitive |  enum   |  mapping  |  value  |  object  |
//   |     T      |    (copy)T | (copy)T |  (copy)T  |(by ref)T|    -     |
//   |     T*     |      -     |    -    |     -     |    -    |(by ref)T?|
//   |  const T*  |      -     |    -    |     -     |    -    |(by ref)T?|
//   |  const T&  |      -     |    -    |     -     |    -    |    -     |
//   |     T&     |      -     |    -    |     -     |    -    |    -     |
//
//   field(script -> native):
//   |            |  primitive |  enum   |  mapping  |  value  |  object  |
//   |     T      |    (copy)T | (copy)T |  (copy)T  | (copy)T |    -     |
//   |     T*     |      -     |    -    |     -     |    -    |(by ref)T?|
//   |  const T*  |      -     |    -    |     -     |    -    |(by ref)T?|
//   |  const T&  |      -     |    -    |     -     |    -    |    -     |
//   |     T&     |      -     |    -    |     -     |    -    |    -     |
//
// about enum export
//   many script has no enum concept, so we need to export enum as an object
//   we recommend export enum as number type, witch is more friendly to native
//
// about parameter In/Out flag:
//   - In: ignore, and always be default
//   - Out: will removed in parameter list, and added in return list
//   - InOut: will appear in both parameter and return list
//
// script support primitive types(must support):
//   void: in return
//   integer: int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t
//   floating: float, double
//   boolean: bool
//   string: skr::String, skr::StringView
//
// about StringView:
//   - cannot pass by ref
//   - cannot used as field
//   - cannot return by script(ScriptMixin)
//   - will copy when return by native
//
// script support generic types(can be toggled off):
//   skr::Array: accept script array
//   skr::Span: accept script array
//   skr::Vector: accept script array
//   skr::Map: accept script map
//   skr::Optional: make primitive/mapping type nullable without use pointer
//   skr::NotNull: make object type non-nullable without use reference
//   skr::FunctionRef: support callback from script for each() methods

namespace skr
{
// StringView export helper
struct StringViewStackProxy {
    StringView view;
    String     holder;

    static void* custom_mapping(void* obj)
    {
        return &reinterpret_cast<StringViewStackProxy*>(obj)->view;
    }
};

// root binder, used for nested binder
struct ScriptBinderPrimitive;
struct ScriptBinderMapping;
struct ScriptBinderObject;
struct ScriptBinderEnum;
struct ScriptBinderValue;
struct ScriptBinderRoot {
    enum class EKind
    {
        None,
        Primitive,
        Value,
        Mapping,
        Object,
        Enum,
    };

    // ctor
    inline ScriptBinderRoot() = default;
    inline ScriptBinderRoot(ScriptBinderPrimitive* primitive)
        : _kind(EKind::Primitive)
        , _binder(primitive)
    {
    }
    inline ScriptBinderRoot(ScriptBinderMapping* mapping)
        : _kind(EKind::Mapping)
        , _binder(mapping)
    {
    }
    inline ScriptBinderRoot(ScriptBinderObject* object)
        : _kind(EKind::Object)
        , _binder(object)
    {
    }
    inline ScriptBinderRoot(ScriptBinderEnum* enum_)
        : _kind(EKind::Enum)
        , _binder(enum_)
    {
    }
    inline ScriptBinderRoot(ScriptBinderValue* value)
        : _kind(EKind::Value)
        , _binder(value)
    {
    }

    // copy & move
    inline ScriptBinderRoot(const ScriptBinderRoot& other) = default;
    inline ScriptBinderRoot(ScriptBinderRoot&& other)      = default;

    // assign & move assign
    inline ScriptBinderRoot& operator=(const ScriptBinderRoot& other) = default;
    inline ScriptBinderRoot& operator=(ScriptBinderRoot&& other)      = default;

    // compare
    inline bool operator==(const ScriptBinderRoot& other) const
    {
        return _kind == other._kind && _binder == other._binder;
    }
    inline bool operator!=(const ScriptBinderRoot& other) const
    {
        return !operator==(other);
    }

    // kind getter
    inline EKind kind() const { return _kind; }
    inline bool  is_empty() const { return _kind == EKind::None; }
    inline bool  is_primitive() const { return _kind == EKind::Primitive; }
    inline bool  is_mapping() const { return _kind == EKind::Mapping; }
    inline bool  is_object() const { return _kind == EKind::Object; }
    inline bool  is_enum() const { return _kind == EKind::Enum; }
    inline bool  is_value() const { return _kind == EKind::Value; }

    // binder getter
    inline ScriptBinderPrimitive* primitive() const
    {
        SKR_ASSERT(_kind == EKind::Primitive);
        return static_cast<ScriptBinderPrimitive*>(_binder);
    }
    inline ScriptBinderMapping* mapping() const
    {
        SKR_ASSERT(_kind == EKind::Mapping);
        return static_cast<ScriptBinderMapping*>(_binder);
    }
    inline ScriptBinderObject* object() const
    {
        SKR_ASSERT(_kind == EKind::Object);
        return static_cast<ScriptBinderObject*>(_binder);
    }
    inline ScriptBinderEnum* enum_() const
    {
        SKR_ASSERT(_kind == EKind::Enum);
        return static_cast<ScriptBinderEnum*>(_binder);
    }
    inline ScriptBinderValue* value() const
    {
        SKR_ASSERT(_kind == EKind::Value);
        return static_cast<ScriptBinderValue*>(_binder);
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
    bool                 failed = false;
};
struct ScriptBinderStaticField {
    const RTTRType*            owner  = nullptr;
    ScriptBinderRoot           binder = {};
    const RTTRStaticFieldData* data   = nullptr;
    bool                       failed = false;
};

// nested binder, method & static method
struct ScriptBinderParam {
    ScriptBinderRoot     binder      = {};
    const RTTRParamData* data        = nullptr;
    bool                 pass_by_ref = false;
    ERTTRParamFlag       inout_flag  = ERTTRParamFlag::None;
    bool                 is_nullable = false;
};
struct ScriptBinderReturn {
    ScriptBinderRoot binder      = {};
    bool             pass_by_ref = false;
    bool             is_nullable = false;
    bool             is_void     = false;
};
struct ScriptBinderMethod {
    struct Overload {
        const RTTRType*           owner         = nullptr;
        const RTTRMethodData*     data          = nullptr;
        ScriptBinderReturn        return_binder = {};
        Vector<ScriptBinderParam> params_binder = {};
        uint32_t                  params_count  = 0;
        uint32_t                  return_count  = 1;
        bool                      failed        = false;
    };

    Vector<Overload> overloads = {};
    bool             failed    = false;
};
struct ScriptBinderStaticMethod {
    struct Overload {
        const RTTRType*             owner         = nullptr;
        const RTTRStaticMethodData* data          = nullptr;
        ScriptBinderReturn          return_binder = {};
        Vector<ScriptBinderParam>   params_binder = {};
        uint32_t                    params_count  = 0;
        uint32_t                    return_count  = 0;
        bool                        failed        = false;
    };

    Vector<Overload> overloads = {};
    bool             failed    = false;
};

// nested binder, property
struct ScriptBinderProperty {
    ScriptBinderMethod setter = {};
    ScriptBinderMethod getter = {};
    bool               failed = false;
};
struct ScriptBinderStaticProperty {
    ScriptBinderStaticMethod setter = {};
    ScriptBinderStaticMethod getter = {};
    bool                     failed = false;
};

// nested binder, constructor
struct ScriptBinderCtor {
    const RTTRCtorData*       data          = nullptr;
    Vector<ScriptBinderParam> params_binder = {};
    bool                      failed        = false;
};

// root binders
struct ScriptBinderPrimitive {
    uint32_t    size      = 0;
    uint32_t    alignment = 0;
    GUID        type_id   = {};
    DtorInvoker dtor      = nullptr;

    bool failed = false;
};
struct ScriptBinderMapping {
    const RTTRType*                type   = nullptr;
    Map<String, ScriptBinderField> fields = {};

    bool failed = false;
};
struct ScriptBinderRecordBase {
    const RTTRType* type = nullptr;

    bool                     is_script_newable = false;
    Vector<ScriptBinderCtor> ctors             = {};

    Map<String, ScriptBinderField>          fields            = {};
    Map<String, ScriptBinderStaticField>    static_fields     = {};
    Map<String, ScriptBinderMethod>         methods           = {};
    Map<String, ScriptBinderStaticMethod>   static_methods    = {};
    Map<String, ScriptBinderProperty>       properties        = {};
    Map<String, ScriptBinderStaticProperty> static_properties = {};

    bool failed = false;
};
struct ScriptBinderObject : ScriptBinderRecordBase {
};
struct ScriptBinderValue : ScriptBinderRecordBase {
};
struct ScriptBinderEnum {
    const RTTRType* type = nullptr;

    ScriptBinderPrimitive* underlying_binder = nullptr;

    Map<String, const RTTREnumItemData*> items = {};

    bool is_signed = false;
};

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
    ScriptBinderMapping*   _make_mapping(const RTTRType* type);
    ScriptBinderObject*    _make_object(const RTTRType* type);
    ScriptBinderValue*     _make_value(const RTTRType* type);
    ScriptBinderEnum*      _make_enum(const RTTRType* type);
    void                   _fill_record_info(ScriptBinderRecordBase& out, const RTTRType* type);

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
    void _make_param(ScriptBinderParam& out, const RTTRParamData* param);
    void _make_return(ScriptBinderReturn& out, TypeSignatureView signature);

    // checker
    void _try_export_field(TypeSignatureView signature, ScriptBinderRoot& out_binder);

private:
    // cache
    Map<GUID, ScriptBinderRoot> _cached_root_binders;

    // logger
    struct Logger {
        struct StackScope {
            StackScope(Logger* logger, String name)
                : _logger(logger)
            {
                _logger->_stack.push_back({ name, false });
            }
            ~StackScope()
            {
                bool err = _logger->any_error();
                _logger->_stack.pop_back();
                if (!_logger->_stack.is_empty())
                {
                    _logger->_stack.back().any_error |= err;
                }
            }

        private:
            Logger* _logger;
        };

        template <typename... Args>
        inline StackScope stack(StringView fmt, Args&&... args)
        {
            return StackScope{
                this,
                format(fmt, std::forward<Args>(args)...)
            };
        }

        template <typename... Args>
        inline void error(StringView fmt, Args&&... args)
        {
            // record error
            if (!_stack.is_empty())
            {
                _stack.back().any_error = true;
            }

            // combine error str
            String error;
            format_to(error, fmt, std::forward<Args>(args)...);
            error.append(u8"\n");
            for (const auto& stack : _stack.range_inv())
            {
                format_to(error, u8"    at: {}\n", stack.name);
            }
            error.first(error.length_buffer() - 1);

            // log error
            SKR_LOG_FMT_ERROR(error.c_str());
        }

        inline bool any_error() const
        {
            if (_stack.is_empty()) { return false; }
            return _stack.back().any_error;
        }

    private:
        struct ErrorStack {
            String name      = {};
            bool   any_error = false;
        };
        Vector<ErrorStack> _stack;
    };
    Logger _logger;
};
} // namespace skr
