#pragma once
#include <SkrRTTR/type_signature.hpp>

namespace skr
{
struct Any {
    using Dtor     = void (*)(void*);
    using CopyFunc = void* (*)(void*);

    // ctor & dtor
    Any();
    template <typename T>
    Any(T&& value);
    ~Any();

    // copy & move
    Any(const Any& rhs);
    Any(Any&& rhs) noexcept;

    // assign & move assign
    Any& operator=(const Any& rhs);
    Any& operator=(Any&& rhs) noexcept;

    // op assign
    template <typename T>
    Any& operator=(T&& value);

    // getter
    bool              has_value() const;
    bool              is_empty() const;
    TypeSignatureView signature() const;

    // ops
    template <typename T>
    void assign(T&& value);
    template <typename T, typename... Args>
    void emplace(Args&&... args);
    void reset();

    // cast
    bool type_is(TypeSignatureView signature, ETypeSignatureCompareFlag compare_flag = ETypeSignatureCompareFlag::Strict) const;
    template <typename T>
    bool type_is(ETypeSignatureCompareFlag compare_flag = ETypeSignatureCompareFlag::Strict) const;
    template <typename T>
    const T* cast(ETypeSignatureCompareFlag compare_flag = ETypeSignatureCompareFlag::Strict) const;
    template <typename T>
    T* cast(ETypeSignatureCompareFlag compare_flag = ETypeSignatureCompareFlag::Strict);
    template <typename T>
    const T& as(ETypeSignatureCompareFlag compare_flag = ETypeSignatureCompareFlag::Strict) const;
    template <typename T>
    T& as(ETypeSignatureCompareFlag compare_flag = ETypeSignatureCompareFlag::Strict);

private:
    // helper
    template <typename T>
    void _fill_func();

private:
    TypeSignature _signature = {};
    void*         _memory    = nullptr;
    CopyFunc      _copy      = nullptr;
    Dtor          _dtor      = nullptr;
};
} // namespace skr

namespace skr
{
// helper
template <typename T>
inline void Any::_fill_func()
{
    _copy = [](void* p) -> void* { return SkrNew<T>(*static_cast<T*>(p)); };
    _dtor = [](void* p) { SkrDelete<T>(static_cast<T*>(p)); };
}

// ctor & dtor
inline Any::Any()
{
}
template <typename T>
inline Any::Any(T&& value)
{
    using RealType = std::decay_t<T>;

    _signature = type_signature_of<RealType>();
    _memory    = SkrNew<RealType>(std::forward<T>(value));
    _fill_func<RealType>();
}
inline Any::~Any()
{
    reset();
}

// copy & move
inline Any::Any(const Any& rhs)
    : _signature(rhs._signature)
    , _memory(rhs._copy(rhs._memory))
    , _copy(rhs._copy)
    , _dtor(rhs._dtor)
{
}
inline Any::Any(Any&& rhs) noexcept
    : _signature(std::move(rhs._signature))
    , _memory(rhs._memory)
    , _copy(rhs._copy)
    , _dtor(rhs._dtor)
{
    rhs._memory = nullptr;
    rhs._copy   = nullptr;
    rhs._dtor   = nullptr;
}

// assign & move assign
inline Any& Any::operator=(const Any& rhs)
{
    if (this != &rhs)
    {
        reset();

        _signature = rhs._signature;
        _memory    = rhs._copy(rhs._memory);
        _copy      = rhs._copy;
        _dtor      = rhs._dtor;
    }
    return *this;
}
inline Any& Any::operator=(Any&& rhs) noexcept
{
    if (this != &rhs)
    {
        reset();

        _signature = std::move(rhs._signature);
        _memory    = rhs._memory;
        _copy      = rhs._copy;
        _dtor      = rhs._dtor;

        rhs._memory = nullptr;
        rhs._copy   = nullptr;
        rhs._dtor   = nullptr;
    }

    return *this;
}

// op assign
template <typename T>
inline Any& Any::operator=(T&& value)
{
    assign<T>(std::forward<T>(value));
}

// getter
inline bool Any::has_value() const
{
    return _memory != nullptr;
}
inline bool Any::is_empty() const
{
    return _memory == nullptr;
}
inline TypeSignatureView Any::signature() const
{
    return _signature;
}

// ops
template <typename T>
inline void Any::assign(T&& value)
{
    reset();

    using RealType = std::decay_t<T>;

    _signature = type_signature_of<RealType>();
    _memory    = SkrNew<RealType>(std::forward<T>(value));
    _fill_func<RealType>();
}
template <typename T, typename... Args>
inline void Any::emplace(Args&&... args)
{
    reset();

    using RealType = std::decay_t<T>;

    _signature = type_signature_of<RealType>();
    _memory    = SkrNew<RealType>(std::forward<Args>(args)...);
    _fill_func<RealType>();
}
inline void Any::reset()
{
    if (_memory)
    {
        _dtor(_memory);
        _signature.reset();
        _memory = nullptr;
        _copy   = nullptr;
        _dtor   = nullptr;
    }
}

// cast
inline bool Any::type_is(TypeSignatureView signature, ETypeSignatureCompareFlag compare_flag) const
{
    if (_memory)
    {
        return _signature.view().equal(signature, compare_flag);
    }
    return false;
}
template <typename T>
inline bool Any::type_is(ETypeSignatureCompareFlag compare_flag) const
{
    TypeSignatureTyped<T> signature;
    return type_is(signature.view(), compare_flag);
}
template <typename T>
inline const T* Any::cast(ETypeSignatureCompareFlag compare_flag) const
{
    if (type_is<T>(compare_flag))
    {
        return reinterpret_cast<const T*>(_memory);
    }
    return nullptr;
}
template <typename T>
inline T* Any::cast(ETypeSignatureCompareFlag compare_flag)
{
    if (type_is<T>(compare_flag))
    {
        return reinterpret_cast<T*>(_memory);
    }
    return nullptr;
}
template <typename T>
inline const T& Any::as(ETypeSignatureCompareFlag compare_flag) const
{
    return *cast<T>(compare_flag);
}
template <typename T>
inline T& Any::as(ETypeSignatureCompareFlag compare_flag)
{
    return *cast<T>(compare_flag);
}

} // namespace skr