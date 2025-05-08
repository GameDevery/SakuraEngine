#pragma once
#include "./sp_util.hpp"
#include <SkrBase/misc/hash.hpp>

namespace skr
{
// 保守 unique pointer 实现，允许协变，但是不处理虚析构情况，禁止 custom deleter，只用于 RAII 的内存管理
template <typename T>
struct UPtr {
    // ctor & dtor
    UPtr();
    UPtr(std::nullptr_t);
    UPtr(T* ptr);
    template <SPConvertible<T> U>
    UPtr(U* ptr);
    ~UPtr();

    // copy & move
    UPtr(const UPtr& rhs) = delete;
    UPtr(UPtr&& rhs);
    template <SPConvertible<T> U>
    UPtr(UPtr<U>&& rhs);

    // assign & move assign
    UPtr& operator=(std::nullptr_t);
    UPtr& operator=(const UPtr& rhs) = delete;
    UPtr& operator=(UPtr&& rhs);
    template <SPConvertible<T> U>
    UPtr& operator=(UPtr<U>&& rhs);

    // factory
    template <typename... Args>
    static UPtr New(Args&&... args);
    template <typename... Args>
    static UPtr NewZeroed(Args&&... args);

    // getter
    T* get() const;

    // empty
    bool is_empty() const;
    operator bool() const;

    // ops
    void reset();
    void reset(T* ptr);
    template <SPConvertible<T> U>
    void reset(U* ptr);
    T*   release();
    void swap(UPtr& rhs);

    // pointer behaviour
    T* operator->() const;
    T& operator*() const;

    // skr hash
    static size_t _skr_hash(const UPtr& obj);

private:
    T* _ptr = nullptr;
};

// shared pointer
template <typename T>
struct SP {
    // ctor & dtor
    SP();
    SP(std::nullptr_t);
    SP(T* ptr);
    template <SPConvertible<T> U>
    SP(U* ptr);
    template <SPConvertible<T> U>
    SP(UPtr<U>&& rhs);
    ~SP();

    // copy & move
    SP(const SP& rhs);
    SP(SP&& rhs);
    template <SPConvertible<T> U>
    SP(const SP<U>& rhs);
    template <SPConvertible<T> U>
    SP(SP<U>&& rhs);

    // assign & move assign
    SP& operator=(std::nullptr_t);
    SP& operator=(T* ptr);
    SP& operator=(const SP& rhs);
    SP& operator=(SP&& rhs);
    template <SPConvertible<T> U>
    SP& operator=(U* ptr);
    template <SPConvertible<T> U>
    SP& operator=(const SP<U>& rhs);
    template <SPConvertible<T> U>
    SP& operator=(SP<U>&& rhs);

    // factory
    template <typename... Args>
    static SP New(Args&&... args);
    template <typename... Args>
    static SP NewZeroed(Args&&... args);

    // getter
    T* get() const;

    // count getter
    SPCounterType  ref_count() const;
    SPCounterType  ref_count_weak() const;
    SPCounterType* get_counter() const;

    // empty
    bool is_empty() const;
    operator bool() const;

    // ops
    void reset();
    void reset(T* ptr);
    template <SPConvertible<T> U>
    void reset(U* ptr);
    void swap(SP& rhs);

    // pointer behaviour
    T* operator->() const;
    T& operator*() const;

    // cast
    template <typename U>
    SP<U> cast_static() const;

    // skr hash
    static size_t _skr_hash(const SP& obj);

private:
    T*            _ptr     = nullptr;
    SPRefCounter* _counter = nullptr;
};

// weak pointer
template <typename T>
struct SPWeak {
    // ctor & dtor
    SPWeak();
    SPWeak(std::nullptr_t);
    SPWeak(T* ptr);
    template <SPConvertible<T> U>
    SPWeak(U* ptr);
    template <SPConvertible<T> U>
    SPWeak(const SP<U>& ptr);
    ~SPWeak();

    // copy & move
    SPWeak(const SPWeak& rhs);
    SPWeak(SPWeak&& rhs);
    template <SPConvertible<T> U>
    SPWeak(const SPWeak<U>& rhs);
    template <SPConvertible<T> U>
    SPWeak(SPWeak<U>&& rhs);

    // assign & move assign
    SPWeak& operator=(std::nullptr_t);
    SPWeak& operator=(T* ptr);
    SPWeak& operator=(const SPWeak& rhs);
    SPWeak& operator=(SPWeak&& rhs);
    template <SPConvertible<T> U>
    SPWeak& operator=(U* ptr);
    template <SPConvertible<T> U>
    SPWeak& operator=(const SPWeak<U>& rhs);
    template <SPConvertible<T> U>
    SPWeak& operator=(SPWeak<U>&& rhs);
    template <SPConvertible<T> U>
    SPWeak& operator=(const SP<U>& rhs);

    // unsafe getter
    T*             get_unsafe() const;
    SPCounterType* get_counter() const;

    // count getter
    SPCounterType ref_count_weak() const;

    // lock
    SP<T> lock() const;

    // empty
    bool is_empty() const;
    bool is_expired() const;
    bool is_alive() const;
    operator bool() const;

    // ops
    void reset();
    void reset(T* ptr);
    template <SPConvertible<T> U>
    void reset(U* ptr);
    template <SPConvertible<T> U>
    void reset(const SP<U>& ptr);
    void swap(SPWeak& rhs);

    // skr hash
    static size_t _skr_hash(const SPWeak& obj);

private:
    T*            _ptr     = nullptr;
    SPRefCounter* _counter = nullptr;
};
} // namespace skr

namespace skr
{
// ctor & dtor
template <typename T>
inline UPtr<T>::UPtr()
{
}
template <typename T>
inline UPtr<T>::UPtr(std::nullptr_t)
{
}
template <typename T>
inline UPtr<T>::UPtr(T* ptr)
    : _ptr(ptr)
{
}
template <typename T>
template <SPConvertible<T> U>
inline UPtr<T>::UPtr(U* ptr)
{
    static_assert(std::is_same_v<U, T> || std::has_virtual_destructor_v<T>, "when use covariance, T must have virtual destructor for safe delete");
    if (ptr)
    {
        _ptr = static_cast<T*>(ptr);
    }
}
template <typename T>
inline UPtr<T>::~UPtr()
{
    reset();
}

// copy & move
template <typename T>
inline UPtr<T>::UPtr(UPtr&& rhs)
    : _ptr(rhs._ptr)
{
    rhs._ptr = nullptr;
}
template <typename T>
template <SPConvertible<T> U>
inline UPtr<T>::UPtr(UPtr<U>&& rhs)
{
    static_assert(std::is_same_v<U, T> || std::has_virtual_destructor_v<T>, "when use covariance, T must have virtual destructor for safe delete");
    if (rhs.get())
    {
        reset(static_cast<T*>(rhs.release()));
    }
}

// assign & move assign
template <typename T>
inline UPtr<T>& UPtr<T>::operator=(std::nullptr_t)
{
    reset();
    return *this;
}
template <typename T>
inline UPtr<T>& UPtr<T>::operator=(UPtr&& rhs)
{
    if (rhs.get())
    {
        reset(rhs._ptr);
        rhs._ptr = nullptr;
    }
    else
    {
        reset();
    }
    return *this;
}
template <typename T>
template <SPConvertible<T> U>
inline UPtr<T>& UPtr<T>::operator=(UPtr<U>&& rhs)
{
    static_assert(std::is_same_v<U, T> || std::has_virtual_destructor_v<T>, "when use covariance, T must have virtual destructor for safe delete");
    if (rhs.get())
    {
        reset(static_cast<T*>(rhs.release()));
    }
    else
    {
        reset();
    }
    return *this;
}

// factory
template <typename T>
template <typename... Args>
inline UPtr<T> UPtr<T>::New(Args&&... args)
{
    return { SkrNew<T>(std::forward<Args>(args)...) };
}
template <typename T>
template <typename... Args>
inline UPtr<T> UPtr<T>::NewZeroed(Args&&... args)
{
    return { SkrNewZeroed<T>(std::forward<Args>(args)...) };
}

// compare
template <typename T>
inline bool operator==(const UPtr<T>& lhs, const UPtr<T>& rhs)
{
    return lhs.get() == rhs.get();
}
template <typename T>
inline bool operator!=(const UPtr<T>& lhs, const UPtr<T>& rhs)
{
    return lhs.get() != rhs.get();
}
template <typename T>
inline bool operator<(const UPtr<T>& lhs, const UPtr<T>& rhs)
{
    return lhs.get() < rhs.get();
}
template <typename T>
inline bool operator>(const UPtr<T>& lhs, const UPtr<T>& rhs)
{
    return lhs.get() > rhs.get();
}
template <typename T>
inline bool operator<=(const UPtr<T>& lhs, const UPtr<T>& rhs)
{
    return lhs.get() <= rhs.get();
}
template <typename T>
inline bool operator>=(const UPtr<T>& lhs, const UPtr<T>& rhs)
{
    return lhs.get() >= rhs.get();
}

// compare with raw pointer (right)
template <typename T>
inline bool operator==(const UPtr<T>& lhs, T* rhs)
{
    return lhs.get() == rhs;
}
template <typename T>
inline bool operator!=(const UPtr<T>& lhs, T* rhs)
{
    return lhs.get() != rhs;
}
template <typename T>
inline bool operator<(const UPtr<T>& lhs, T* rhs)
{
    return lhs.get() < rhs;
}
template <typename T>
inline bool operator>(const UPtr<T>& lhs, T* rhs)
{
    return lhs.get() > rhs;
}
template <typename T>
inline bool operator<=(const UPtr<T>& lhs, T* rhs)
{
    return lhs.get() <= rhs;
}
template <typename T>
inline bool operator>=(const UPtr<T>& lhs, T* rhs)
{
    return lhs.get() >= rhs;
}

// compare with raw pointer (left)
template <typename T>
inline bool operator==(T* lhs, const UPtr<T>& rhs)
{
    return lhs == rhs.get();
}
template <typename T>
inline bool operator!=(T* lhs, const UPtr<T>& rhs)
{
    return lhs != rhs.get();
}
template <typename T>
inline bool operator<(T* lhs, const UPtr<T>& rhs)
{
    return lhs < rhs.get();
}
template <typename T>
inline bool operator>(T* lhs, const UPtr<T>& rhs)
{
    return lhs > rhs.get();
}
template <typename T>
inline bool operator<=(T* lhs, const UPtr<T>& rhs)
{
    return lhs <= rhs.get();
}
template <typename T>
inline bool operator>=(T* lhs, const UPtr<T>& rhs)
{
    return lhs >= rhs.get();
}

// compare with nullptr
template <typename T>
inline bool operator==(const UPtr<T>& lhs, std::nullptr_t)
{
    return lhs.get() == nullptr;
}
template <typename T>
inline bool operator!=(const UPtr<T>& lhs, std::nullptr_t)
{
    return lhs.get() != nullptr;
}
template <typename T>
inline bool operator==(std::nullptr_t, const UPtr<T>& rhs)
{
    return nullptr == rhs.get();
}
template <typename T>
inline bool operator!=(std::nullptr_t, const UPtr<T>& rhs)
{
    return nullptr != rhs.get();
}

// getter
template <typename T>
inline T* UPtr<T>::get() const
{
    return _ptr;
}

// empty
template <typename T>
inline bool UPtr<T>::is_empty() const
{
    return _ptr == nullptr;
}
template <typename T>
inline UPtr<T>::operator bool() const
{
    return !is_empty();
}

// ops
template <typename T>
inline void UPtr<T>::reset()
{
    if (_ptr)
    {
        SPDeleterTraits<T>::do_delete(_ptr);
        _ptr = nullptr;
    }
}
template <typename T>
inline void UPtr<T>::reset(T* ptr)
{
    if (_ptr != ptr)
    {
        if (_ptr)
        {
            SPDeleterTraits<T>::do_delete(_ptr);
        }
        _ptr = ptr;
    }
}
template <typename T>
template <SPConvertible<T> U>
inline void UPtr<T>::reset(U* ptr)
{
    static_assert(std::is_same_v<U, T> || std::has_virtual_destructor_v<T>, "when use covariance, T must have virtual destructor for safe delete");
    if (ptr)
    {
        reset(static_cast<T*>(ptr));
    }
    else
    {
        reset();
    }
}
template <typename T>
inline T* UPtr<T>::release()
{
    if (_ptr)
    {
        T* tmp = _ptr;
        _ptr   = nullptr;
        return tmp;
    }
    return nullptr;
}
template <typename T>
inline void UPtr<T>::swap(UPtr& rhs)
{
    if (this != &rhs)
    {
        T* tmp   = _ptr;
        _ptr     = rhs._ptr;
        rhs._ptr = tmp;
    }
}

// pointer behaviour
template <typename T>
inline T* UPtr<T>::operator->() const
{
    return _ptr;
}
template <typename T>
inline T& UPtr<T>::operator*() const
{
    return *_ptr;
}

// skr hash
template <typename T>
inline size_t UPtr<T>::_skr_hash(const UPtr& obj)
{
    return skr::Hash<T*>()(obj._ptr);
}
} // namespace skr