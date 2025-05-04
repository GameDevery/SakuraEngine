#pragma once
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
    while (!counter.compare_exchange_strong(old, old + 1, std::memory_order_relaxed))
    {
        SKR_ASSERT(!rc_is_unique(old) && "try to add ref on a unique object");
    }
    return old + 1;
}
inline static RCCounterType rc_add_ref_unique(std::atomic<RCCounterType>& counter)
{
    RCCounterType old = counter.load(std::memory_order_relaxed);
    SKR_ASSERT(old == 0 && "try to add ref on a non-unique object");
    while (!counter.compare_exchange_strong(old, kRCCounterUniqueFlag, std::memory_order_relaxed))
    {
        SKR_ASSERT(old == 0 && "try to add ref on a non-unique object");
    }
    return kRCCounterUniqueFlag;
}
inline static RCCounterType rc_release(std::atomic<RCCounterType>& counter)
{
    RCCounterType old = counter.fetch_sub(1, std::memory_order_relaxed);
    if (rc_is_unique(old))
    {
        return 0;
    }
    else
    {
        return old - 1;
    }
}

// concept
template <typename T>
concept ObjectWithRC = requires(T obj) {
    { obj.skr_rc_add_ref() } -> std::same_as<RCCounterType>;
    { obj.skr_rc_add_ref_unique() } -> std::same_as<RCCounterType>;
    { obj.skr_rc_release() } -> std::same_as<RCCounterType>;
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
#define SKR_RC_INTEFACE()                                   \
    virtual skr::RCCounterType skr_rc_add_ref()        = 0; \
    virtual skr::RCCounterType skr_rc_add_ref_unique() = 0; \
    virtual skr::RCCounterType skr_rc_release()        = 0;
#define SKR_RC_DELETER_INTERFACE() \
    virtual void skr_rc_delete() = 0;

// impl macros
#define SKR_RC_IMPL()                                                                               \
private:                                                                                            \
    inline ::std::atomic<::skr::RCCounterType> zz_skr_rc = 0;                                       \
                                                                                                    \
public:                                                                                             \
    inline skr::RCCounterType skr_rc_add_ref() { return skr::rc_add_ref(zz_skr_rc); }               \
    inline skr::RCCounterType skr_rc_add_ref_unique() { return skr::rc_add_ref_unique(zz_skr_rc); } \
    inline skr::RCCounterType skr_rc_release() { return skr::rc_release(zz_skr_rc); }
