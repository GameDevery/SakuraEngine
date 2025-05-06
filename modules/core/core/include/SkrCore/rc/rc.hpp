#pragma once
#include "./rc_util.hpp"
#include <SkrBase/misc/hash.hpp>

namespace skr
{
template <ObjectWithRC T>
struct RC;
template <ObjectWithRC T>
struct RCWeak;
template <ObjectWithRC T>
struct RCUnique;
template <ObjectWithRC T>
struct RCWeakLocker;

template <ObjectWithRC T>
struct RC {
    friend struct RCWeakLocker<T>;

    // ctor & dtor
    RC();
    RC(std::nullptr_t);
    RC(T* ptr);
    ~RC();

    // copy & move
    template <ObjectWithRCConvertible<T> U>
    RC(const RC<U>& rhs);
    template <ObjectWithRCConvertible<T> U>
    RC(RC<U>&& rhs);
    RC(const RC& rhs);
    RC(RC&& rhs);

    // assign & move assign
    RC& operator=(std::nullptr_t);
    RC& operator=(T* ptr);
    template <ObjectWithRCConvertible<T> U>
    RC& operator=(const RC<U>& rhs);
    template <ObjectWithRCConvertible<T> U>
    RC& operator=(RC<U>&& rhs);
    RC& operator=(const RC& rhs);
    RC& operator=(RC&& rhs);

    // factory
    template <typename... Args>
    static RC New(Args&&... args);
    template <typename... Args>
    static RC NewZeroed(Args&&... args);

    // getter
    T* get() const;

    // count getter
    RCCounterType ref_count() const;
    RCCounterType ref_count_weak() const;

    // empty
    bool is_empty() const;
    operator bool() const;

    // ops
    void reset();
    void reset(T* ptr);
    void swap(RC& rhs);

    // pointer behaviour
    T* operator->() const;
    T& operator*() const;

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
    // ctor & dtor
    RCUnique();
    RCUnique(std::nullptr_t);
    RCUnique(T* ptr);
    ~RCUnique();

    // copy & move
    RCUnique(const RCUnique& rhs) = delete;
    template <ObjectWithRCConvertible<T> U>
    RCUnique(RCUnique<U>&& rhs);
    RCUnique(RCUnique&& rhs);

    // assign & move assign
    RCUnique& operator=(std::nullptr_t);
    RCUnique& operator=(T* ptr);
    RCUnique& operator=(const RCUnique& rhs) = delete;
    template <ObjectWithRCConvertible<T> U>
    RCUnique& operator=(RCUnique<U>&& rhs);
    RCUnique& operator=(RCUnique&& rhs);

    // factory
    template <typename... Args>
    static RCUnique New(Args&&... args);
    template <typename... Args>
    static RCUnique NewZeroed(Args&&... args);

    // getter
    T* get() const;

    // count getter
    RCCounterType ref_count() const;
    RCCounterType ref_count_weak() const;

    // empty
    bool is_empty() const;
    operator bool() const;

    // ops
    void reset();
    void reset(T* ptr);
    T*   release();
    void swap(RCUnique& rhs);

    // pointer behaviour
    T* operator->() const;
    T& operator*() const;

    // skr hash
    static size_t _skr_hash(const RCUnique& obj);

private:
    // helper
    void _release();

private:
    T* _ptr = nullptr;
};

template <ObjectWithRC T>
struct RCWeakLocker {
    // ctor & dtor
    RCWeakLocker(T* ptr, RCWeakRefCounter* counter);
    ~RCWeakLocker();

    // copy & move
    RCWeakLocker(const RCWeakLocker& rhs) = delete;
    RCWeakLocker(RCWeakLocker&& rhs);

    // assign & move assign
    RCWeakLocker& operator=(const RCWeakLocker& rhs) = delete;
    RCWeakLocker& operator=(RCWeakLocker&& rhs);

    // is empty
    bool is_empty() const;
    operator bool() const;

    // getter
    T* get() const;

    // pointer behaviour
    T* operator->() const;
    T& operator*() const;

    // lock to RC
    RC<T> rc() const;
    operator RC<T>() const;

private:
    T*                _ptr;
    RCWeakRefCounter* _counter;
};

template <ObjectWithRC T>
struct RCWeak {
    // ctor & dtor
    RCWeak();
    RCWeak(std::nullptr_t);
    RCWeak(T* ptr);
    template <ObjectWithRCConvertible<T> U>
    RCWeak(const RC<U>& ptr);
    template <ObjectWithRCConvertible<T> U>
    RCWeak(const RCUnique<U>& ptr);
    ~RCWeak();

    // copy & move
    template <ObjectWithRCConvertible<T> U>
    RCWeak(const RCWeak<U>& rhs);
    template <ObjectWithRCConvertible<T> U>
    RCWeak(RCWeak<U>&& rhs);
    RCWeak(const RCWeak& rhs);
    RCWeak(RCWeak&& rhs);

    // assign & move assign
    RCWeak& operator=(std::nullptr_t);
    RCWeak& operator=(T* ptr);
    template <ObjectWithRCConvertible<T> U>
    RCWeak& operator=(const RCWeak<U>& rhs);
    template <ObjectWithRCConvertible<T> U>
    RCWeak& operator=(RCWeak<U>&& rhs);
    template <ObjectWithRCConvertible<T> U>
    RCWeak& operator=(const RC<U>& rhs);
    template <ObjectWithRCConvertible<T> U>
    RCWeak& operator=(const RCUnique<U>& rhs);
    RCWeak& operator=(const RCWeak& rhs);
    RCWeak& operator=(RCWeak&& rhs);

    // unsafe getter
    T*                get_unsafe() const;
    RCWeakRefCounter* get_counter() const;

    // count getter
    RCCounterType ref_count_weak() const;

    // lock
    RCWeakLocker<T> lock() const;

    // empty
    bool is_empty() const;
    bool is_expired() const;
    bool is_alive() const;
    operator bool() const;

    // ops
    void reset();
    void reset(T* ptr);
    template <ObjectWithRCConvertible<T> U>
    void reset(const RC<U>& ptr);
    template <ObjectWithRCConvertible<T> U>
    void reset(const RCUnique<U>& ptr);
    void swap(RCWeak& rhs);

    // skr hash
    static size_t _skr_hash(const RCWeak& obj);

private:
    // helper
    void _release();
    void _take_weak_ref_counter();

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
            _ptr->skr_rc_delete();
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
inline RC<T>::RC(T* ptr)
    : _ptr(ptr)
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
{
    if (!rhs.is_empty())
    {
        reset(static_cast<T*>(rhs.get()));
    }
}
template <ObjectWithRC T>
template <ObjectWithRCConvertible<T> U>
inline RC<T>::RC(RC<U>&& rhs)
{
    if (!rhs.is_empty())
    {
        reset(static_cast<T*>(rhs.get()));
        rhs.reset();
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
inline RC<T>& RC<T>::operator=(std::nullptr_t)
{
    reset();
    return *this;
}
template <ObjectWithRC T>
inline RC<T>& RC<T>::operator=(T* ptr)
{
    reset(ptr);
    return *this;
}
template <ObjectWithRC T>
template <ObjectWithRCConvertible<T> U>
inline RC<T>& RC<T>::operator=(const RC<U>& rhs)
{
    if (rhs.is_empty())
    {
        reset();
    }
    else
    {
        reset(static_cast<T*>(rhs.get()));
    }
    return *this;
}

template <ObjectWithRC T>
template <ObjectWithRCConvertible<T> U>
inline RC<T>& RC<T>::operator=(RC<U>&& rhs)
{
    if (rhs.is_empty())
    {
        reset();
    }
    else
    {
        reset(static_cast<T*>(rhs.get()));
        rhs.reset();
    }
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
inline bool operator==(const RC<T>& lhs, const RC<U>& rhs)
{
    return lhs.get() == rhs.get();
}
template <ObjectWithRC T, ObjectWithRC U>
inline bool operator!=(const RC<T>& lhs, const RC<U>& rhs)
{
    return lhs.get() != rhs.get();
}
template <ObjectWithRC T, ObjectWithRC U>
inline bool operator<(const RC<T>& lhs, const RC<U>& rhs)
{
    return lhs.get() < rhs.get();
}
template <ObjectWithRC T, ObjectWithRC U>
inline bool operator>(const RC<T>& lhs, const RC<U>& rhs)
{
    return lhs.get() > rhs.get();
}
template <ObjectWithRC T, ObjectWithRC U>
inline bool operator<=(const RC<T>& lhs, const RC<U>& rhs)
{
    return lhs.get() <= rhs.get();
}
template <ObjectWithRC T, ObjectWithRC U>
inline bool operator>=(const RC<T>& lhs, const RC<U>& rhs)
{
    return lhs.get() >= rhs.get();
}

// compare with nullptr
template <ObjectWithRC T>
inline bool operator==(const RC<T>& lhs, std::nullptr_t)
{
    return lhs.get() == nullptr;
}
template <ObjectWithRC T>
inline bool operator!=(const RC<T>& lhs, std::nullptr_t)
{
    return lhs.get() != nullptr;
}
template <ObjectWithRC T>
inline bool operator==(std::nullptr_t, const RC<T>& rhs)
{
    return nullptr == rhs.get();
}
template <ObjectWithRC T>
inline bool operator!=(std::nullptr_t, const RC<T>& rhs)
{
    return nullptr != rhs.get();
}

// getter
template <ObjectWithRC T>
inline T* RC<T>::get() const
{
    return _ptr;
}

// count getter
template <ObjectWithRC T>
inline RCCounterType RC<T>::ref_count() const
{
    return _ptr ? _ptr->skr_rc_count() : 0;
}
template <ObjectWithRC T>
inline RCCounterType RC<T>::ref_count_weak() const
{
    return _ptr ? _ptr->skr_rc_weak_ref_count() : 0;
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
    return !is_empty();
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
    if (this != &rhs)
    {
        T* tmp   = _ptr;
        _ptr     = rhs._ptr;
        rhs._ptr = tmp;
    }
}

// pointer behaviour
template <ObjectWithRC T>
inline T* RC<T>::operator->() const
{
    return _ptr;
}
template <ObjectWithRC T>
inline T& RC<T>::operator*() const
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

// impl for RCUnique
namespace skr
{
// helper
template <ObjectWithRC T>
inline void RCUnique<T>::_release()
{
    SKR_ASSERT(_ptr != nullptr);
    if (_ptr->skr_rc_release() == 0)
    {
        _ptr->skr_rc_weak_ref_counter_notify_dead();
        if constexpr (ObjectWithRCDeleter<T>)
        {
            _ptr->skr_rc_delete();
        }
        else
        {
            SkrDelete(_ptr);
        }
    }
}

// ctor & dtor
template <ObjectWithRC T>
inline RCUnique<T>::RCUnique()
{
}
template <ObjectWithRC T>
inline RCUnique<T>::RCUnique(std::nullptr_t)
{
}
template <ObjectWithRC T>
inline RCUnique<T>::RCUnique(T* ptr)
    : _ptr(ptr)
{
    if (_ptr)
    {
        _ptr->skr_rc_add_ref_unique();
    }
}
template <ObjectWithRC T>
inline RCUnique<T>::~RCUnique()
{
    reset();
}

// copy & move
template <ObjectWithRC T>
template <ObjectWithRCConvertible<T> U>
inline RCUnique<T>::RCUnique(RCUnique<U>&& rhs)
{
    if (!rhs.is_empty())
    {
        reset(static_cast<T*>(rhs.release()));
    }
}
template <ObjectWithRC T>
inline RCUnique<T>::RCUnique(RCUnique&& rhs)
    : _ptr(rhs._ptr)
{
    rhs._ptr = nullptr;
}

// assign & move assign
template <ObjectWithRC T>
inline RCUnique<T>& RCUnique<T>::operator=(std::nullptr_t)
{
    reset();
    return *this;
}
template <ObjectWithRC T>
inline RCUnique<T>& RCUnique<T>::operator=(T* ptr)
{
    reset(ptr);
    return *this;
}
template <ObjectWithRC T>
template <ObjectWithRCConvertible<T> U>
inline RCUnique<T>& RCUnique<T>::operator=(RCUnique<U>&& rhs)
{
    if (rhs.is_empty())
    {
        reset();
    }
    else
    {
        reset(static_cast<T*>(rhs.release()));
    }
    return *this;
}
template <ObjectWithRC T>
inline RCUnique<T>& RCUnique<T>::operator=(RCUnique&& rhs)
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
inline RCUnique<T> RCUnique<T>::New(Args&&... args)
{
    return { SkrNew<T>(std::forward<Args>(args)...) };
}
template <ObjectWithRC T>
template <typename... Args>
inline RCUnique<T> RCUnique<T>::NewZeroed(Args&&... args)
{
    return { SkrNewZeroed<T>(std::forward<Args>(args)...) };
}

// compare
template <ObjectWithRC T, ObjectWithRC U>
inline bool operator==(const RCUnique<T>& lhs, const RCUnique<U>& rhs)
{
    return lhs.get() == rhs.get();
}
template <ObjectWithRC T, ObjectWithRC U>
inline bool operator!=(const RCUnique<T>& lhs, const RCUnique<U>& rhs)
{
    return lhs.get() != rhs.get();
}
template <ObjectWithRC T, ObjectWithRC U>
inline bool operator<(const RCUnique<T>& lhs, const RCUnique<U>& rhs)
{
    return lhs.get() < rhs.get();
}
template <ObjectWithRC T, ObjectWithRC U>
inline bool operator>(const RCUnique<T>& lhs, const RCUnique<U>& rhs)
{
    return lhs.get() > rhs.get();
}
template <ObjectWithRC T, ObjectWithRC U>
inline bool operator<=(const RCUnique<T>& lhs, const RCUnique<U>& rhs)
{
    return lhs.get() <= rhs.get();
}
template <ObjectWithRC T, ObjectWithRC U>
inline bool operator>=(const RCUnique<T>& lhs, const RCUnique<U>& rhs)
{
    return lhs.get() >= rhs.get();
}

// compare with nullptr
template <ObjectWithRC T>
inline bool operator==(const RCUnique<T>& lhs, std::nullptr_t)
{
    return lhs.get() == nullptr;
}
template <ObjectWithRC T>
inline bool operator!=(const RCUnique<T>& lhs, std::nullptr_t)
{
    return lhs.get() != nullptr;
}
template <ObjectWithRC T>
inline bool operator==(std::nullptr_t, const RCUnique<T>& rhs)
{
    return nullptr == rhs.get();
}
template <ObjectWithRC T>
inline bool operator!=(std::nullptr_t, const RCUnique<T>& rhs)
{
    return nullptr != rhs.get();
}

// getter
template <ObjectWithRC T>
inline T* RCUnique<T>::get() const
{
    return _ptr;
}

// count getter
template <ObjectWithRC T>
inline RCCounterType RCUnique<T>::ref_count() const
{
    return _ptr ? _ptr->skr_rc_count() : 0;
}
template <ObjectWithRC T>
inline RCCounterType RCUnique<T>::ref_count_weak() const
{
    return _ptr ? _ptr->skr_rc_weak_ref_count() : 0;
}

// empty
template <ObjectWithRC T>
inline bool RCUnique<T>::is_empty() const
{
    return _ptr == nullptr;
}
template <ObjectWithRC T>
inline RCUnique<T>::operator bool() const
{
    return !is_empty();
}

// ops
template <ObjectWithRC T>
inline void RCUnique<T>::reset()
{
    if (_ptr)
    {
        _release();
        _ptr = nullptr;
    }
}
template <ObjectWithRC T>
inline void RCUnique<T>::reset(T* ptr)
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
            _ptr->skr_rc_add_ref_unique();
        }
    }
}
template <ObjectWithRC T>
inline T* RCUnique<T>::release()
{
    if (_ptr)
    {
        T* tmp = _ptr;
        _ptr->skr_rc_release_unique();
        _ptr = nullptr;
        return tmp;
    }
    else
    {
        return nullptr;
    }
}
template <ObjectWithRC T>
inline void RCUnique<T>::swap(RCUnique& rhs)
{
    if (this != &rhs)
    {
        T* tmp   = _ptr;
        _ptr     = rhs._ptr;
        rhs._ptr = tmp;
    }
}

// pointer behaviour
template <ObjectWithRC T>
inline T* RCUnique<T>::operator->() const
{
    return _ptr;
}
template <ObjectWithRC T>
inline T& RCUnique<T>::operator*() const
{
    return *_ptr;
}

// skr hash
template <ObjectWithRC T>
inline size_t RCUnique<T>::_skr_hash(const RCUnique& obj)
{
    return skr::Hash<T*>()(obj._ptr);
}
} // namespace skr

// impl for RCWeakLocker
namespace skr
{
// ctor & dtor
template <ObjectWithRC T>
inline RCWeakLocker<T>::RCWeakLocker(T* ptr, RCWeakRefCounter* counter)
    : _ptr(ptr)
    , _counter(counter)
{
    if (counter && counter->is_alive())
    {
        _counter->lock_for_use();
        if (counter->is_alive())
        { // success lock
            return;
        }
        else
        { // failed lock
            _counter->unlock_for_use();
        }
    }

    // failed lock, reset ptr
    _ptr     = nullptr;
    _counter = nullptr;
}
template <ObjectWithRC T>
inline RCWeakLocker<T>::~RCWeakLocker()
{
    if (_ptr)
    {
        _counter->unlock_for_use();
    }
}

// copy & move
template <ObjectWithRC T>
inline RCWeakLocker<T>::RCWeakLocker(RCWeakLocker&& rhs)
    : _ptr(rhs._ptr)
    , _counter(rhs._counter)
{
    rhs._ptr     = nullptr;
    rhs._counter = nullptr;
}

// assign & move assign
template <ObjectWithRC T>
inline RCWeakLocker<T>& RCWeakLocker<T>::operator=(RCWeakLocker&& rhs)
{
    if (this != &rhs)
    {
        if (_ptr)
        {
            _counter->unlock_for_use();
        }

        _ptr         = rhs._ptr;
        _counter     = rhs._counter;
        rhs._ptr     = nullptr;
        rhs._counter = nullptr;
    }
    return *this;
}

// is empty
template <ObjectWithRC T>
inline bool RCWeakLocker<T>::is_empty() const
{
    return _ptr == nullptr;
}
template <ObjectWithRC T>
inline RCWeakLocker<T>::operator bool() const
{
    return !is_empty();
}

// getter
template <ObjectWithRC T>
inline T* RCWeakLocker<T>::get() const
{
    return _ptr;
}

// pointer behaviour
template <ObjectWithRC T>
inline T* RCWeakLocker<T>::operator->() const
{
    return _ptr;
}
template <ObjectWithRC T>
inline T& RCWeakLocker<T>::operator*() const
{
    return *_ptr;
}

// lock to RC
template <ObjectWithRC T>
inline RC<T> RCWeakLocker<T>::rc() const
{
    RC<T> result;
    if (_ptr)
    {
        auto lock_result = _ptr->skr_rc_weak_lock();
        if (lock_result != 0)
        {
            result._ptr = _ptr;
        }
    }
    return result;
}
template <ObjectWithRC T>
inline RCWeakLocker<T>::operator RC<T>() const
{
    return rc();
}

} // namespace skr

// impl for RCWeak
namespace skr
{
// helper
template <ObjectWithRC T>
inline void RCWeak<T>::_release()
{
    SKR_ASSERT(_ptr != nullptr);
    _counter->release();
}
template <ObjectWithRC T>
inline void RCWeak<T>::_take_weak_ref_counter()
{
    SKR_ASSERT(_ptr != nullptr);
    _counter = _ptr->skr_rc_weak_ref_counter();
    SKR_ASSERT(_counter != nullptr);
    _counter->add_ref();
}

// ctor & dtor
template <ObjectWithRC T>
inline RCWeak<T>::RCWeak()
{
}
template <ObjectWithRC T>
inline RCWeak<T>::RCWeak(std::nullptr_t)
{
}
template <ObjectWithRC T>
inline RCWeak<T>::RCWeak(T* ptr)
    : _ptr(ptr)
{
    if (_ptr)
    {
        _take_weak_ref_counter();
    }
}
template <ObjectWithRC T>
template <ObjectWithRCConvertible<T> U>
inline RCWeak<T>::RCWeak(const RC<U>& ptr)
    : _ptr(static_cast<T*>(ptr.get()))
{
    if (_ptr)
    {
        _take_weak_ref_counter();
    }
}
template <ObjectWithRC T>
template <ObjectWithRCConvertible<T> U>
inline RCWeak<T>::RCWeak(const RCUnique<U>& ptr)
    : _ptr(static_cast<T*>(ptr.get()))
{
    if (_ptr)
    {
        _take_weak_ref_counter();
    }
}
template <ObjectWithRC T>
inline RCWeak<T>::~RCWeak()
{
    reset();
}

// copy & move
template <ObjectWithRC T>
template <ObjectWithRCConvertible<T> U>
inline RCWeak<T>::RCWeak(const RCWeak<U>& rhs)
{
    if (rhs.is_alive())
    {
        _ptr     = static_cast<T*>(rhs.get_unsafe());
        _counter = rhs.get_counter();
        _counter->add_ref();
    }
}
template <ObjectWithRC T>
template <ObjectWithRCConvertible<T> U>
inline RCWeak<T>::RCWeak(RCWeak<U>&& rhs)
{
    if (rhs.is_alive())
    {
        _ptr     = static_cast<T*>(rhs.get_unsafe());
        _counter = rhs.get_counter();
        _counter->add_ref();
        rhs.reset();
    }
}
template <ObjectWithRC T>
inline RCWeak<T>::RCWeak(const RCWeak& rhs)
    : _ptr(rhs._ptr)
    , _counter(rhs._counter)
{
    if (_counter)
    {
        _counter->add_ref();
    }
}
template <ObjectWithRC T>
inline RCWeak<T>::RCWeak(RCWeak&& rhs)
    : _ptr(rhs._ptr)
    , _counter(rhs._counter)
{
    rhs._ptr     = nullptr;
    rhs._counter = nullptr;
}

// assign & move assign
template <ObjectWithRC T>
inline RCWeak<T>& RCWeak<T>::operator=(std::nullptr_t)
{
    reset();
    return *this;
}
template <ObjectWithRC T>
inline RCWeak<T>& RCWeak<T>::operator=(T* ptr)
{
    reset(ptr);
    return *this;
}
template <ObjectWithRC T>
template <ObjectWithRCConvertible<T> U>
inline RCWeak<T>& RCWeak<T>::operator=(const RCWeak<U>& rhs)
{
    reset();
    if (rhs.is_alive())
    {
        _ptr     = static_cast<T*>(rhs.get_unsafe());
        _counter = rhs.get_counter();
        _counter->add_ref();
    }
    return *this;
}
template <ObjectWithRC T>
template <ObjectWithRCConvertible<T> U>
inline RCWeak<T>& RCWeak<T>::operator=(RCWeak<U>&& rhs)
{
    reset();
    if (rhs.is_alive())
    {
        _ptr     = static_cast<T*>(rhs.get_unsafe());
        _counter = rhs.get_counter();
        _counter->add_ref();
        rhs.reset();
    }
    return *this;
}
template <ObjectWithRC T>
template <ObjectWithRCConvertible<T> U>
inline RCWeak<T>& RCWeak<T>::operator=(const RC<U>& rhs)
{
    reset(rhs);
    return *this;
}
template <ObjectWithRC T>
template <ObjectWithRCConvertible<T> U>
inline RCWeak<T>& RCWeak<T>::operator=(const RCUnique<U>& rhs)
{
    reset(rhs);
    return *this;
}
template <ObjectWithRC T>
inline RCWeak<T>& RCWeak<T>::operator=(const RCWeak& rhs)
{
    if (this != &rhs)
    {
        reset(rhs._ptr);
    }
    return *this;
}
template <ObjectWithRC T>
inline RCWeak<T>& RCWeak<T>::operator=(RCWeak&& rhs)
{
    if (this != &rhs)
    {
        reset();
        _ptr         = rhs._ptr;
        _counter     = rhs._counter;
        rhs._ptr     = nullptr;
        rhs._counter = nullptr;
    }
    return *this;
}

// compare
template <ObjectWithRC T, ObjectWithRC U>
inline bool operator==(const RCWeak<T>& lhs, const RCWeak<U>& rhs)
{
    return lhs.get_unsafe() == rhs.get_unsafe() && lhs.get_counter() == rhs.get_counter();
}
template <ObjectWithRC T, ObjectWithRC U>
inline bool operator!=(const RCWeak<T>& lhs, const RCWeak<U>& rhs)
{
    return lhs.get_unsafe() != rhs.get_unsafe() || lhs.get_counter() != rhs.get_counter();
}
template <ObjectWithRC T, ObjectWithRC U>
inline bool operator<(const RCWeak<T>& lhs, const RCWeak<U>& rhs)
{
    if (lhs.get_unsafe() != rhs.get_unsafe()) return lhs.get_unsafe() < rhs.get_unsafe();
    return lhs.get_counter() < rhs.get_counter();
}
template <ObjectWithRC T, ObjectWithRC U>
inline bool operator>(const RCWeak<T>& lhs, const RCWeak<U>& rhs)
{
    if (lhs.get_unsafe() != rhs.get_unsafe()) return lhs.get_unsafe() > rhs.get_unsafe();
    return lhs.get_counter() > rhs.get_counter();
}
template <ObjectWithRC T, ObjectWithRC U>
inline bool operator<=(const RCWeak<T>& lhs, const RCWeak<U>& rhs)
{
    return !(lhs > rhs);
}
template <ObjectWithRC T, ObjectWithRC U>
inline bool operator>=(const RCWeak<T>& lhs, const RCWeak<U>& rhs)
{
    return !(lhs < rhs);
}

// compare with nullptr
template <ObjectWithRC T>
inline bool operator==(const RCWeak<T>& lhs, std::nullptr_t)
{
    return lhs.get_unsafe() == nullptr;
}
template <ObjectWithRC T>
inline bool operator!=(const RCWeak<T>& lhs, std::nullptr_t)
{
    return lhs.get_unsafe() != nullptr;
}
template <ObjectWithRC T>
inline bool operator==(std::nullptr_t, const RCWeak<T>& rhs)
{
    return nullptr == rhs.get_unsafe();
}
template <ObjectWithRC T>
inline bool operator!=(std::nullptr_t, const RCWeak<T>& rhs)
{
    return nullptr != rhs.get_unsafe();
}

// unsafe getter
template <ObjectWithRC T>
inline T* RCWeak<T>::get_unsafe() const
{
    return _ptr;
}
template <ObjectWithRC T>
inline RCWeakRefCounter* RCWeak<T>::get_counter() const
{
    return _counter;
}

// count getter
template <ObjectWithRC T>
inline RCCounterType RCWeak<T>::ref_count_weak() const
{
    return _counter ? _counter->ref_count() : 0;
}

// lock
template <ObjectWithRC T>
inline RCWeakLocker<T> RCWeak<T>::lock() const
{
    return { _ptr, _counter };
}

// empty
template <ObjectWithRC T>
inline bool RCWeak<T>::is_empty() const
{
    return _ptr == nullptr;
}
template <ObjectWithRC T>
inline bool RCWeak<T>::is_expired() const
{
    return !is_alive();
}
template <ObjectWithRC T>
inline bool RCWeak<T>::is_alive() const
{
    if (_ptr == nullptr) { return false; }
    return _counter->is_alive();
}
template <ObjectWithRC T>
inline RCWeak<T>::operator bool() const
{
    return is_alive();
}

// ops
template <ObjectWithRC T>
inline void RCWeak<T>::reset()
{
    if (_ptr)
    {
        _release();
        _ptr     = nullptr;
        _counter = nullptr;
    }
}
template <ObjectWithRC T>
inline void RCWeak<T>::reset(T* ptr)
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
            _take_weak_ref_counter();
        }
    }
}
template <ObjectWithRC T>
template <ObjectWithRCConvertible<T> U>
inline void RCWeak<T>::reset(const RC<U>& ptr)
{
    reset(static_cast<T*>(ptr.get()));
}
template <ObjectWithRC T>
template <ObjectWithRCConvertible<T> U>
inline void RCWeak<T>::reset(const RCUnique<U>& ptr)
{
    reset(static_cast<T*>(ptr.get()));
}
template <ObjectWithRC T>
inline void RCWeak<T>::swap(RCWeak& rhs)
{
    if (this != &rhs)
    {
        T*                     tmp_ptr     = _ptr;
        skr::RCWeakRefCounter* tmp_counter = _counter;
        _ptr                               = rhs._ptr;
        _counter                           = rhs._counter;
        rhs._ptr                           = tmp_ptr;
        rhs._counter                       = tmp_counter;
    }
}

// skr hash
template <ObjectWithRC T>
inline size_t RCWeak<T>::_skr_hash(const RCWeak& obj)
{
    return hash_combine(
        skr::Hash<T*>()(obj._ptr),
        skr::Hash<skr::RCWeakRefCounter*>()(obj._counter)
    );
}

} // namespace skr