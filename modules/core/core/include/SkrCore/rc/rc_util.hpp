#pragma once
#include <SkrCore/memory/memory.h>
#include <SkrBase/config.h>
#include <atomic>
#include <SkrBase/types/expected.hpp>
#include <concepts>

namespace skr
{
using RCCounterType = uint32_t;

// constants
inline static constexpr RCCounterType kRCCounterUniqueFlag = 1 << 31;
inline static constexpr RCCounterType kRCCounterMax        = kRCCounterUniqueFlag - 1;

// operations
inline static RCCounterType rc_get(const std::atomic<RCCounterType>& counter)
{
    return counter.load(std::memory_order_relaxed);
}
inline static bool rc_is_unique(RCCounterType counter)
{
    return (counter & kRCCounterUniqueFlag) != 0;
}
inline static RCCounterType rc_add_ref(std::atomic<RCCounterType>& counter)
{
    RCCounterType old = counter.load(std::memory_order_relaxed);
    SKR_ASSERT(!rc_is_unique(old) && "try to add ref on a unique object");
    while (!counter.compare_exchange_strong(
        old,
        old + 1,
        std::memory_order_relaxed
    ))
    {
        SKR_ASSERT(!rc_is_unique(old) && "try to add ref on a unique object");
    }
    return old + 1;
}
inline static RCCounterType rc_add_ref_unique(std::atomic<RCCounterType>& counter)
{
    RCCounterType old = counter.load(std::memory_order_relaxed);
    SKR_ASSERT(old == 0 && "try to add ref on a non-unique object");
    while (!counter.compare_exchange_strong(
        old,
        kRCCounterUniqueFlag,
        std::memory_order_relaxed
    ))
    {
        SKR_ASSERT(old == 0 && "try to add ref on a non-unique object");
    }
    return kRCCounterUniqueFlag;
}
inline static RCCounterType rc_release(std::atomic<RCCounterType>& counter)
{
    RCCounterType old = counter.fetch_sub(1, std::memory_order_release);
    if (rc_is_unique(old))
    {
        return 0;
    }
    else
    {
        return old - 1;
    }
}

// weak block
struct RCWeakRefCounter {
    inline RCCounterType ref_count() const
    {
        return _ref_count.load(std::memory_order_relaxed);
    }
    inline void add_ref()
    {
        _ref_count.fetch_add(1, std::memory_order_relaxed);
    }
    inline void release()
    {
        SKR_ASSERT(_ref_count.load(std::memory_order_relaxed) > 0);
        if (_ref_count.fetch_sub(1, std::memory_order_release) == 1)
        {
            std::atomic_thread_fence(std::memory_order_acquire);
            SkrDelete(this);
        }
    }
    inline void notify_dead()
    {
        SKR_ASSERT(_is_alive.load(std::memory_order_relaxed) == true);
        _is_alive.store(false, std::memory_order_relaxed);
    }
    inline bool is_alive() const
    {
        return _is_alive.load(std::memory_order_relaxed);
    }

private:
    // just store ref count and live info
    // true object pointer will stored by RCWeak
    std::atomic<bool>          _is_alive  = true;
    std::atomic<RCCounterType> _ref_count = 0;
};
inline static RCWeakRefCounter* rc_get_weak_ref_released()
{
    return reinterpret_cast<RCWeakRefCounter*>(size_t(-1));
}
inline static bool rc_is_weak_ref_released(RCWeakRefCounter* counter)
{
    return reinterpret_cast<size_t>(counter) == size_t(-1);
}
inline static RCWeakRefCounter* rc_get_or_new_weak_ref_counter(
    std::atomic<RCWeakRefCounter*>& counter
)
{
    RCWeakRefCounter* weak_counter = counter.load(std::memory_order_relaxed);
    if (weak_counter)
    {
        return rc_is_weak_ref_released(weak_counter) ? nullptr : weak_counter;
    }
    else
    {
        RCWeakRefCounter* new_counter = SkrNew<RCWeakRefCounter>();
        if (counter.compare_exchange_strong(
                weak_counter,
                new_counter,
                std::memory_order_release
            ))
        {
            std::atomic_thread_fence(std::memory_order_acquire);
            // add ref for keep it alive until any weak ref and self released
            new_counter->add_ref();
            return new_counter;
        }
        else
        {
            std::atomic_thread_fence(std::memory_order_acquire);
            if (rc_is_weak_ref_released(weak_counter))
            {
                // object has been released, delete the new one
                SkrDelete(new_counter);
                return nullptr;
            }
            else
            {
                // another thread created a weak counter, delete the new one
                SkrDelete(new_counter);
                return weak_counter;
            }
        }
    }
}
inline static void rc_notify_weak_ref_counter_dead(
    std::atomic<RCWeakRefCounter*>& counter
)
{
    RCWeakRefCounter* weak_counter = counter.load(std::memory_order_relaxed);

    // take release permissions
    while (!counter.compare_exchange_strong(
        weak_counter,
        rc_get_weak_ref_released(),
        std::memory_order_release
    ))
    {
        if (rc_is_weak_ref_released(weak_counter))
        {
            // another thread released the weak counter
            return;
        }
        else
        {
            // unexpected, should not happen
            SKR_UNREACHABLE_CODE();
        }
    }

    // now release the weak counter
    if (weak_counter)
    {
        std::atomic_thread_fence(std::memory_order_acquire);
        weak_counter->notify_dead();
        weak_counter->release();
        counter.store(nullptr, std::memory_order_relaxed);
    }
}

// concept
template <typename T>
concept ObjectWithRC = requires(T obj) {
    { obj.skr_rc_add_ref() } -> std::same_as<RCCounterType>;
    { obj.skr_rc_add_ref_unique() } -> std::same_as<RCCounterType>;
    { obj.skr_rc_release() } -> std::same_as<RCCounterType>;
    { obj.skr_rc_weak_ref_counter() } -> std::same_as<RCWeakRefCounter*>;
    { obj.skr_rc_weak_ref_counter_notify_dead() } -> std::same_as<void>;
};
template <typename T>
concept ObjectWithRCDeleter = requires(T obj) {
    { obj.skr_rc_delete() } -> std::same_as<void>;
};
template <typename From, typename To>
concept ObjectWithRCConvertible = requires(From obj) {
    ObjectWithRC<From>;
    std::convertible_to<From*, To*>;
};
} // namespace skr

// interface macros
#define SKR_RC_INTEFACE()                                                     \
    virtual skr::RCCounterType     skr_rc_add_ref()                      = 0; \
    virtual skr::RCCounterType     skr_rc_add_ref_unique()               = 0; \
    virtual skr::RCCounterType     skr_rc_release()                      = 0; \
    virtual skr::RCWeakRefCounter* skr_rc_weak_ref_counter()             = 0; \
    virtual void                   skr_rc_weak_ref_counter_notify_dead() = 0;
#define SKR_RC_DELETER_INTERFACE() \
    virtual void skr_rc_delete() = 0;

// impl macros
#define SKR_RC_IMPL()                                                             \
private:                                                                          \
    inline ::std::atomic<::skr::RCCounterType>     zz_skr_rc           = 0;       \
    inline ::std::atomic<::skr::RCWeakRefCounter*> zz_skr_weak_counter = nullptr; \
                                                                                  \
public:                                                                           \
    inline skr::RCCounterType skr_rc_add_ref()                                    \
    {                                                                             \
        return skr::rc_add_ref(zz_skr_rc);                                        \
    }                                                                             \
    inline skr::RCCounterType skr_rc_add_ref_unique()                             \
    {                                                                             \
        return skr::rc_add_ref_unique(zz_skr_rc);                                 \
    }                                                                             \
    inline skr::RCCounterType skr_rc_release()                                    \
    {                                                                             \
        return skr::rc_release(zz_skr_rc);                                        \
    }                                                                             \
    inline skr::RCWeakRefCounter* skr_rc_weak_ref_counter()                       \
    {                                                                             \
        return skr::rc_get_or_new_weak_ref_counter(zz_skr_weak_counter);          \
    }                                                                             \
    inline void skr_rc_weak_ref_counter_notify_dead()                             \
    {                                                                             \
        skr::rc_notify_weak_ref_counter_dead(zz_skr_weak_counter);                \
    }
