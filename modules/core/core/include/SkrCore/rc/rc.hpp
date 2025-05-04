#pragma once
#include "./rc_util.hpp"
#include <SkrBase/misc/hash.hpp>

namespace skr
{
template <ObjectWithRC T>
struct RC {
    // ctor & dtor
    RC();
    RC(std::nullptr_t);
    template <ObjectWithRCConvertible<T> U>
    RC(U* ptr);
    ~RC();

    // copy & move
    template <ObjectWithRCConvertible<T> U>
    RC(const RC<U>& rhs);
    RC(const RC& rhs);
    RC(RC&& rhs);

    // assign & move assign
    template <ObjectWithRCConvertible<T> U>
    RC& operator=(const RC<U>& rhs);
    template <ObjectWithRCConvertible<T> U>
    RC& operator=(U* ptr);
    RC& operator=(const RC& rhs);
    RC& operator=(RC&& rhs);

    // factory
    template <typename... Args>
    static RC New(Args&&... args);
    template <typename... Args>
    static RC NewZeroed(Args&&... args);

    // compare
    //! Note: use T/U compare, see above for details
    // friend bool operator==(const RC& lhs, const RC& rhs);
    // friend bool operator!=(const RC& lhs, const RC& rhs);
    // friend bool operator<(const RC& lhs, const RC& rhs);
    // friend bool operator>(const RC& lhs, const RC& rhs);
    // friend bool operator<=(const RC& lhs, const RC& rhs);
    // friend bool operator>=(const RC& lhs, const RC& rhs);

    // compare with nullptr
    friend bool operator==(const RC& lhs, std::nullptr_t);
    friend bool operator!=(const RC& lhs, std::nullptr_t);
    friend bool operator==(std::nullptr_t, const RC& rhs);
    friend bool operator!=(std::nullptr_t, const RC& rhs);

    // getter
    T*       get();
    const T* get() const;

    // empty
    bool is_empty() const;
    operator bool() const;

    // ops
    void reset();
    void reset(T* ptr);
    void swap(RC& rhs);

    // pointer behaviour
    T*       operator->();
    const T* operator->() const;
    T&       operator*();
    const T& operator*() const;

    // skr hash
    static size_t _skr_hash(const RC& obj);

private:
    // helper
    void _release();

private:
    T* _ptr = nullptr;
};
template <ObjectWithRC T>
struct RCUnique {

private:
    T* _ptr = nullptr;
};
template <ObjectWithRC T>
struct RCWeak {

private:
    T*                     _ptr     = nullptr;
    skr::RCWeakRefCounter* _counter = nullptr;
};
} // namespace skr

// impl for RC
namespace skr
{
// helper
template <ObjectWithRC T>
inline void RC<T>::_release()
{
    SKR_ASSERT(_ptr != nullptr);
    if (_ptr->skr_rc_release() == 0)
    {
        _ptr->skr_rc_weak_ref_counter_notify_dead();
        if constexpr (ObjectWithRCDeleter<T>)
        {
            _ptr->skr_rc_release();
        }
        else
        {
            SkrDelete(_ptr);
        }
    }
}

// ctor & dtor
template <ObjectWithRC T>
inline RC<T>::RC()
{
}
template <ObjectWithRC T>
inline RC<T>::RC(std::nullptr_t)
{
}
template <ObjectWithRC T>
template <ObjectWithRCConvertible<T> U>
inline RC<T>::RC(U* ptr)
    : _ptr(static_cast<T*>(ptr))
{
    if (_ptr)
    {
        _ptr->skr_rc_add_ref();
    }
}
template <ObjectWithRC T>
inline RC<T>::~RC()
{
    reset();
}

// copy & move
template <ObjectWithRC T>
template <ObjectWithRCConvertible<T> U>
inline RC<T>::RC(const RC<U>& rhs)
    : _ptr(static_cast<T*>(rhs._ptr))
{
    if (_ptr)
    {
        _ptr->skr_rc_add_ref();
    }
}
template <ObjectWithRC T>
inline RC<T>::RC(const RC& rhs)
    : _ptr(rhs._ptr)
{
    if (_ptr)
    {
        _ptr->skr_rc_add_ref();
    }
}
template <ObjectWithRC T>
inline RC<T>::RC(RC&& rhs)
    : _ptr(rhs._ptr)
{
    rhs._ptr = nullptr;
}

// assign & move assign
template <ObjectWithRC T>
template <ObjectWithRCConvertible<T> U>
inline RC<T>& RC<T>::operator=(const RC<U>& rhs)
{
    reset(static_cast<T*>(rhs._ptr));
    return *this;
}
template <ObjectWithRC T>
template <ObjectWithRCConvertible<T> U>
inline RC<T>& RC<T>::operator=(U* ptr)
{
    reset(static_cast<T*>(ptr));
    return *this;
}
template <ObjectWithRC T>
inline RC<T>& RC<T>::operator=(const RC& rhs)
{
    if (this != &rhs)
    {
        reset(rhs._ptr);
    }
    return *this;
}
template <ObjectWithRC T>
inline RC<T>& RC<T>::operator=(RC&& rhs)
{
    if (this != &rhs)
    {
        reset();
        _ptr     = rhs._ptr;
        rhs._ptr = nullptr;
    }
    return *this;
}

// factory
template <ObjectWithRC T>
template <typename... Args>
inline RC<T> RC<T>::New(Args&&... args)
{
    return { SkrNew<T>(std::forward<Args>(args)...) };
}
template <ObjectWithRC T>
template <typename... Args>
inline RC<T> RC<T>::NewZeroed(Args&&... args)
{
    return { SkrNewZeroed<T>(std::forward<Args>(args)...) };
}

// compare
template <ObjectWithRC T, ObjectWithRC U>
inline bool operator==(const RC<T>& lhs, const RC<T>& rhs)
{
    return lhs._ptr == rhs._ptr;
}
template <ObjectWithRC T, ObjectWithRC U>
inline bool operator!=(const RC<T>& lhs, const RC<T>& rhs)
{
    return lhs._ptr != rhs._ptr;
}
template <ObjectWithRC T, ObjectWithRC U>
inline bool operator<(const RC<T>& lhs, const RC<T>& rhs)
{
    return lhs._ptr < rhs._ptr;
}
template <ObjectWithRC T, ObjectWithRC U>
inline bool operator>(const RC<T>& lhs, const RC<T>& rhs)
{
    return lhs._ptr > rhs._ptr;
}
template <ObjectWithRC T, ObjectWithRC U>
inline bool operator<=(const RC<T>& lhs, const RC<T>& rhs)
{
    return lhs._ptr <= rhs._ptr;
}
template <ObjectWithRC T, ObjectWithRC U>
inline bool operator>=(const RC<T>& lhs, const RC<T>& rhs)
{
    return lhs._ptr >= rhs._ptr;
}

// compare with nullptr
template <ObjectWithRC T>
inline bool operator==(const RC<T>& lhs, std::nullptr_t)
{
    return lhs._ptr == nullptr;
}
template <ObjectWithRC T>
inline bool operator!=(const RC<T>& lhs, std::nullptr_t)
{
    return lhs._ptr != nullptr;
}
template <ObjectWithRC T>
inline bool operator==(std::nullptr_t, const RC<T>& rhs)
{
    return nullptr == rhs._ptr;
}
template <ObjectWithRC T>
inline bool operator!=(std::nullptr_t, const RC<T>& rhs)
{
    return nullptr != rhs._ptr;
}

// getter
template <ObjectWithRC T>
inline T* RC<T>::get()
{
    return _ptr;
}
template <ObjectWithRC T>
inline const T* RC<T>::get() const
{
    return _ptr;
}

// empty
template <ObjectWithRC T>
inline bool RC<T>::is_empty() const
{
    return _ptr == nullptr;
}
template <ObjectWithRC T>
inline RC<T>::operator bool() const
{
    return _ptr != nullptr;
}

// ops
template <ObjectWithRC T>
inline void RC<T>::reset()
{
    if (_ptr)
    {
        _release();
        _ptr = nullptr;
    }
}
template <ObjectWithRC T>
inline void RC<T>::reset(T* ptr)
{
    if (_ptr != ptr)
    {
        // release old ptr
        if (_ptr)
        {
            _release();
        }

        // add ref to new ptr
        _ptr = ptr;
        if (_ptr)
        {
            _ptr->skr_rc_add_ref();
        }
    }
}
template <ObjectWithRC T>
inline void RC<T>::swap(RC& rhs)
{
    T* tmp   = _ptr;
    _ptr     = rhs._ptr;
    rhs._ptr = tmp;
}

// pointer behaviour
template <ObjectWithRC T>
inline T* RC<T>::operator->()
{
    return _ptr;
}
template <ObjectWithRC T>
inline const T* RC<T>::operator->() const
{
    return _ptr;
}
template <ObjectWithRC T>
inline T& RC<T>::operator*()
{
    return *_ptr;
}
template <ObjectWithRC T>
inline const T& RC<T>::operator*() const
{
    return *_ptr;
}

// skr hash
template <ObjectWithRC T>
inline size_t RC<T>::_skr_hash(const RC& obj)
{
    return skr::Hash<T*>()(obj._ptr);
}

} // namespace skr