#pragma once
#include <SkrBase/atomic/atomic_mutex.hpp>
#include <SkrCore/memory/memory.h>
#include <SkrBase/config.h>
#include <atomic>
#include <SkrBase/types/expected.hpp>
#include <concepts>

namespace skr
{
using SPCounterType = uint32_t;
template <typename T, typename Deleter>
concept SPDeleter = requires(T* p, Deleter deleter) {
    { deleter(p) } -> std::same_as<void>;
};
template <typename U, typename T>
concept SPConvertible = std::convertible_to<U*, T*>;
template <typename T>
struct SPDeleterDefault {
    inline void operator()(T* p) const SKR_NOEXCEPT
    {
        SkrDelete(p);
    }
};

//========================SP ref counter========================
struct SPRefCounter {
    SPCounterType ref_count() const;
    void          add_ref();
    void          release();
    void          add_ref_weak();
    void          release_weak();
    bool          try_lock_weak();

    virtual void delete_value()   = 0;
    virtual void delete_counter() = 0;

private:
    std::atomic<SPCounterType> _ref_count      = 0;
    std::atomic<SPCounterType> _ref_count_weak = 0;
};
template <typename T, SPDeleter<T> Deleter = SPDeleterDefault<T>>
struct SPRefCounterImpl : SPRefCounter {
    // ctor & dtor
    SPRefCounterImpl(T* ptr);
    SPRefCounterImpl(T* ptr, Deleter deleter);
    ~SPRefCounterImpl();

    // impl delete interface
    void delete_value() override;
    void delete_counter() override;

private:
    T*      _ptr = nullptr;
    Deleter _deleter;
};

//========================unique pointer========================
template <typename T>
concept ObjectWithUPtrCustomDeleter = requires(T* p) {
    { p->skr_uptr_delete() } -> std::same_as<void>;
};
template <typename From, typename To>
concept UPtrConvertible = requires() {
    std::convertible_to<From*, To*>;
};
template <typename T>
struct UPtrDeleterTraits {
    static void do_delete(T* p);
};
template <ObjectWithUPtrCustomDeleter T>
struct UPtrDeleterTraits<T> {
    static void do_delete(T* p);
};
// 保守 unique pointer 实现，允许协变，但是不处理虚析构情况，禁止 custom deleter，只用于 RAII 的内存管理
template <typename T>
struct UPtr {
    // ctor & dtor
    UPtr();
    UPtr(std::nullptr_t);
    UPtr(T* ptr);
    template <UPtrConvertible<T> U>
    UPtr(U* ptr);
    ~UPtr();

    // copy & move
    UPtr(const UPtr& rhs) = delete;
    UPtr(UPtr&& rhs);
    template <UPtrConvertible<T> U>
    UPtr(UPtr<U>&& rhs) = delete;

    // assign & move assign
    UPtr& operator=(const UPtr& rhs) = delete;
    UPtr& operator=(UPtr&& rhs);
    template <UPtrConvertible<T> U>
    UPtr& operator=(UPtr<U>&& rhs) = delete;

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
    template <UPtrConvertible<T> U>
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

} // namespace skr

// impl for SPRefCounter
namespace skr
{
inline SPCounterType SPRefCounter::ref_count() const
{
    return _ref_count.load(std::memory_order_relaxed);
}
inline void SPRefCounter::add_ref()
{
    _ref_count.fetch_add(1, std::memory_order_relaxed);
    _ref_count_weak.fetch_add(1, std::memory_order_relaxed);
}
inline void SPRefCounter::release()
{
    SKR_ASSERT(_ref_count.load(std::memory_order_relaxed) > 0);
    if (_ref_count.fetch_sub(1, std::memory_order_release) == 1)
    {
        atomic_thread_fence(std::memory_order_acquire);
        delete_value();
    }

    release_weak();
}
inline void SPRefCounter::add_ref_weak()
{
    _ref_count_weak.fetch_add(1, std::memory_order_relaxed);
}
inline void SPRefCounter::release_weak()
{
    SKR_ASSERT(_ref_count_weak.load(std::memory_order_relaxed) > 0);
    if (_ref_count_weak.fetch_sub(1, std::memory_order_release) == 1)
    {
        atomic_thread_fence(std::memory_order_acquire);
        delete_counter();
    }
}
inline bool SPRefCounter::try_lock_weak()
{
    for (SPCounterType old = _ref_count.load(std::memory_order_relaxed); old != 0;)
    {
        if (_ref_count.compare_exchange_weak(
                old,
                old + 1,
                std::memory_order_relaxed
            ))
        {
            _ref_count_weak.fetch_add(1, std::memory_order_relaxed);
            return true;
        }
    }
    return false;
}

// ctor & dtor
template <typename T, SPDeleter<T> Deleter>
inline SPRefCounterImpl<T, Deleter>::SPRefCounterImpl(T* ptr)
    : _ptr(ptr)
    , _deleter()
{
}
template <typename T, SPDeleter<T> Deleter>
inline SPRefCounterImpl<T, Deleter>::SPRefCounterImpl(T* ptr, Deleter deleter)
    : _ptr(ptr)
    , _deleter(std::move(deleter))
{
}
template <typename T, SPDeleter<T> Deleter>
inline SPRefCounterImpl<T, Deleter>::~SPRefCounterImpl() = default;

// impl delete interface
template <typename T, SPDeleter<T> Deleter>
inline void SPRefCounterImpl<T, Deleter>::delete_value()
{
    _deleter(_ptr);
    _ptr = nullptr;
}
template <typename T, SPDeleter<T> Deleter>
inline void SPRefCounterImpl<T, Deleter>::delete_counter()
{
    SkrDelete(this);
}
} // namespace skr