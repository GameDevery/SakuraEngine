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
// basic binder, used for nested binder
struct PrimitiveBinder;
struct BoxBinder;
struct WrapBinder;
struct BasicBinder {
    enum class EKind
    {
        None,
        Primitive,
        Box,
        Wrap,
    };

    // ctor
    inline BasicBinder() = default;
    inline BasicBinder(PrimitiveBinder* primitive)
        : _kind(EKind::Primitive)
        , _binder(primitive)
    {
    }
    inline BasicBinder(BoxBinder* box)
        : _kind(EKind::Box)
        , _binder(box)
    {
    }
    inline BasicBinder(WrapBinder* wrap)
        : _kind(EKind::Wrap)
        , _binder(wrap)
    {
    }

    // copy & move
    inline BasicBinder(const BasicBinder& other) = default;
    inline BasicBinder(BasicBinder&& other)      = default;

    // assign & move assign
    inline BasicBinder& operator=(const BasicBinder& other) = default;
    inline BasicBinder& operator=(BasicBinder&& other)      = default;

    // kind getter
    inline EKind kind() const { return _kind; }
    inline bool  is_empty() const { return _kind == EKind::None; }
    inline bool  is_primitive() const { return _kind == EKind::Primitive; }
    inline bool  is_box() const { return _kind == EKind::Box; }
    inline bool  is_wrap() const { return _kind == EKind::Wrap; }

    // binder getter
    inline PrimitiveBinder* primitive() const
    {
        SKR_ASSERT(_kind == EKind::Primitive);
        return static_cast<PrimitiveBinder*>(_binder);
    }
    inline BoxBinder* box() const
    {
        SKR_ASSERT(_kind == EKind::Box);
        return static_cast<BoxBinder*>(_binder);
    }
    inline WrapBinder* wrap() const
    {
        SKR_ASSERT(_kind == EKind::Wrap);
        return static_cast<WrapBinder*>(_binder);
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
struct FieldBinder {
    RTTRType*      owner  = nullptr;
    BasicBinder    binder = {};
    RTTRFieldData* data   = nullptr;
};
struct StaticFieldBinder {
    RTTRType*            owner  = nullptr;
    BasicBinder          binder = {};
    RTTRStaticFieldData* data   = nullptr;
};

// nested binder, method & static method
struct ParamBinder {
    BasicBinder binder      = {};
    bool        is_inout    = false;
    bool        is_nullable = false;
};
struct ReturnBinder {
    BasicBinder binder      = {};
    bool        is_nullable = false;
};
struct MethodBinder {
    struct Overload {
        RTTRType*           owner         = nullptr;
        RTTRMethodData*     data          = nullptr;
        ReturnBinder        return_binder = {};
        Vector<ParamBinder> params_binder = {};
    };

    Vector<Overload> overloads = {};
};
struct StaticMethodBinder {
    struct Overload {
        RTTRType*             owner         = nullptr;
        RTTRStaticMethodData* data          = nullptr;
        ReturnBinder          return_binder = {};
        Vector<ParamBinder>   params_binder = {};
    };

    Vector<Overload> overloads = {};
};

// root binders
struct PrimitiveBinder {
    uint32_t    size      = 0;
    uint32_t    alignment = 0;
    DtorInvoker dtor      = nullptr;
    GUID        type_id   = {};
};
struct BoxBinder {
    RTTRType*                type;
    Map<String, FieldBinder> fields;
};
struct WrapBinder {
    RTTRType* type;

    Map<String, FieldBinder>        fields;
    Map<String, StaticFieldBinder>  static_fields;
    Map<String, MethodBinder>       methods;
    Map<String, StaticMethodBinder> static_methods;
};

struct BinderManager {

private:
    // cache
    Map<GUID, BasicBinder> _cached_basic_binders;
};
} // namespace skr
