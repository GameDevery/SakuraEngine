#pragma once
#include <SkrBase/atomic/atomic_mutex.hpp>
#include <SkrCore/memory/memory.h>
#include <SkrBase/config.h>
#include <atomic>
#include <SkrBase/types/expected.hpp>
#include <concepts>

namespace skr
{
//========================SP ref counter========================
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

struct SPRefCounter {
    inline SPCounterType ref_count() const
    {
        return _ref_count.load(std::memory_order_relaxed);
    }
    inline void add_ref()
    {
        _ref_count.fetch_add(1, std::memory_order_relaxed);
        _ref_count_weak.fetch_add(1, std::memory_order_relaxed);
    }
    inline void release()
    {
        SKR_ASSERT(_ref_count.load(std::memory_order_relaxed) > 0);
        if (_ref_count.fetch_sub(1, std::memory_order_release) == 1)
        {
            atomic_thread_fence(std::memory_order_acquire);
            delete_value();
        }

        release_weak();
    }
    inline void add_ref_weak()
    {
        _ref_count_weak.fetch_add(1, std::memory_order_relaxed);
    }
    inline void release_weak()
    {
        SKR_ASSERT(_ref_count_weak.load(std::memory_order_relaxed) > 0);
        if (_ref_count_weak.fetch_sub(1, std::memory_order_release) == 1)
        {
            atomic_thread_fence(std::memory_order_acquire);
            delete_counter();
        }
    }
    inline bool try_lock_weak()
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

    virtual void delete_value()   = 0;
    virtual void delete_counter() = 0;

private:
    std::atomic<SPCounterType> _ref_count      = 0;
    std::atomic<SPCounterType> _ref_count_weak = 0;
};
template <typename T, SPDeleter<T> Deleter = SPDeleterDefault<T>>
struct SPRefCounterImpl : SPRefCounter {
    // ctor & dtor
    inline SPRefCounterImpl(T* ptr)
        : _ptr(ptr)
        , _deleter()
    {
    }
    inline SPRefCounterImpl(T* ptr, Deleter deleter)
        : _ptr(ptr)
        , _deleter(std::move(deleter))
    {
    }
    inline ~SPRefCounterImpl() = default;

    // impl delete interface
    inline void delete_value() override
    {
        _deleter(_ptr);
        _ptr = nullptr;
    }
    inline void delete_counter() override
    {
        SkrDelete(this);
    }

private:
    T*      _ptr = nullptr;
    Deleter _deleter;
};

//========================unique pointer========================
template <typename T>
concept ObjectWithUPtrDeleter = requires(T* p) {
    { p->skr_uptr_delete() } -> std::same_as<void>;
};
template <typename From, typename To>
concept UPtrConvertible = requires() {
    std::convertible_to<From*, To*>;
};
template <typename T>
struct UPtrDeleterTraits {
    inline static void do_delete(T* p)
    {
        SkrDelete(p);
    }
};
template <ObjectWithUPtrDeleter T>
struct UPtrDeleterTraits<T> {
    inline static void do_delete(T* p)
    {
        p->skr_uptr_delete();
    }
};

} // namespace skr