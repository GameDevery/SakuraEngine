#pragma once
#include "SkrCore/memory/memory.h"
#include "SkrTask/fib_task.hpp"
#include <algorithm>
#include <iterator>
/*
The api parallel_for and concurrent is similar but distinguishable, due to marl's dispatch philosophy, scheduled tasks scramble physical logic core. Thus "parallel_for" schedule tasks as much as arguments' required to ensure later tasks' responding performance. But "concurrent" schedule fixed tasks to ensure less stack-frame pointers' cutover for maximum computing performance, which may cause later tasks' latency.
*/
namespace skr
{
namespace concepts
{
template <typename Iter>
concept is_iterator = requires(Iter ite, size_t n) {
    *ite;
    !std::is_integral_v<Iter>;
    std::distance(ite, ite);
    std::advance(ite, n);
};
} // namespace concepts
template <typename Iter>
[[nodiscard]] auto _distance(Iter ite, Iter last)
{
    if constexpr (skr::concepts::is_iterator<std::remove_cvref_t<Iter>>)
    {
        return std::distance(ite, last);
    }
    else
    {
        return last > ite ? last - ite : ite - last;
    }
}
template <typename Iter>
void _advance(Iter&& ite, size_t n)
{
    if constexpr (skr::concepts::is_iterator<std::remove_cvref_t<Iter>>)
    {
        std::advance(ite, n);
    }
    else
    {
        ite += n;
    }
}
template <class F, class Iter>
requires(std::is_invocable_v<F, Iter, Iter>)
void parallel_for(Iter begin, Iter end, size_t batch, F f, size_t inplace_batch_threahold = 1ull)
{
    auto   n          = _distance(begin, end);
    size_t batchCount = (n + batch - 1) / batch;
    if (batchCount < inplace_batch_threahold)
    {
        for (size_t i = 0; i < batchCount; ++i)
        {
            auto toAdvance = std::min((size_t)n, batch);
            auto l         = begin;
            auto r         = begin;
            _advance(r, toAdvance);
            n -= toAdvance;
            f(l, r);
            begin = r;
        }
    }
    else
    {
        task::counter_t counter;
        counter.add((uint32_t)batchCount);
        for (size_t i = 0; i < batchCount; ++i)
        {
            auto toAdvance = std::min((size_t)n, batch);
            auto l         = begin;
            auto r         = begin;
            _advance(r, toAdvance);
            n -= toAdvance;
            skr::task::schedule([counter, f, l, r]() mutable {
                SKR_DEFER({ counter.decrement(); });
                f(l, r);
            },
                                nullptr);
            begin = r;
        }
        counter.wait(true);
    }
}

template <class F, class Iter>
requires(std::is_invocable_v<F, Iter, Iter>)
void serial_for(Iter begin, Iter end, size_t batch, F f)
{
    parallel_for(begin, end, batch, f, UINT32_MAX);
}

template <class F, class Iter>
requires(std::is_invocable_v<F, Iter, Iter>)
void parallel_for(task::counter_t& counter, Iter begin, Iter end, size_t batch, F f, size_t inplace_batch_threahold = 1ull)
{
    auto   n          = _distance(begin, end);
    size_t batchCount = (n + batch - 1) / batch;
    if (batchCount < inplace_batch_threahold)
    {
        for (size_t i = 0; i < batchCount; ++i)
        {
            auto toAdvance = std::min((size_t)n, batch);
            auto l         = begin;
            auto r         = begin;
            _advance(r, toAdvance);
            n -= toAdvance;
            f(l, r);
            begin = r;
        }
    }
    else
    {
        counter.add((uint32_t)batchCount);
        for (size_t i = 0; i < batchCount; ++i)
        {
            auto toAdvance = std::min((size_t)n, batch);
            auto l         = begin;
            auto r         = begin;
            _advance(r, toAdvance);
            n -= toAdvance;
            skr::task::schedule([counter, f, l, r]() mutable {
                SKR_DEFER({ counter.decrement(); });
                f(l, r);
            },
                                nullptr);
            begin = r;
        }
    }
}
namespace fiber_detail
{
template <typename T>
struct NonMovableAtomic {
    std::atomic<T> value;
    NonMovableAtomic() = default;
    NonMovableAtomic(T&& t)
        : value{ std::move(t) }
    {
    }
    NonMovableAtomic(NonMovableAtomic const&) = delete;
    NonMovableAtomic(NonMovableAtomic&& rhs)
        : value{ rhs.value.load() }
    {
    }
};
template <typename F>
struct SharedFunc;
template <typename Ret, typename... Args>
struct SharedFunc<Ret(Args...)> final {
    struct alignas(16) Closure {
        std::atomic_size_t ref;
        Ret (*func_ptr)(void*, Args&&...);
        Ret (*dtor)(void*);
    };
    Closure* ptr;
    template <typename F>
    requires(std::is_invocable_v<F, Args && ...> && !std::is_same_v<std::remove_cvref_t<F>, SharedFunc>)
    SharedFunc(F&& f)
    {
        using PureFunc = std::remove_cvref_t<F>;
        ptr            = new (sakura_malloc(sizeof(PureFunc) + sizeof(Closure))) Closure{};
        ptr->ref       = 1;
        ptr->func_ptr  = [](void* ptr, Args&&... args) -> Ret {
            return (*reinterpret_cast<PureFunc*>(ptr))(std::forward<Args>(args)...);
        };
        ptr->dtor = [](void* ptr) {
            reinterpret_cast<PureFunc*>(ptr)->~PureFunc();
        };
        new (std::launder(ptr + 1)) PureFunc{ std::forward<F>(f) };
    }
    Ret operator()(Args... args) const
    {
        return ptr->func_ptr(ptr + 1, std::forward<Args>(args)...);
    }
    SharedFunc(SharedFunc const& rhs)
    {
        ptr = rhs.ptr;
        if (ptr)
        {
            ptr->ref++;
        }
    }
    SharedFunc(SharedFunc&& rhs)
        : ptr{ rhs.ptr }

    {
        rhs.ptr = nullptr;
    }
    ~SharedFunc()
    {
        if (ptr)
        {
            if (--ptr->ref == 0)
            {
                ptr->dtor(std::launder(ptr + 1));
                sakura_free(ptr);
            }
        }
    }
};

} // namespace fiber_detail
template <class F, class Iter>
requires(std::is_invocable_v<F, Iter, Iter>)
[[nodiscard]] task::counter_t async_concurrent(Iter begin, Iter end, size_t batch, F f)
{
    auto            n          = _distance(begin, end);
    size_t          batchCount = (n + batch - 1) / batch;
    task::counter_t counter;
    auto            scheduleCount = std::min<size_t>(batchCount, std::thread::hardware_concurrency());
    counter.add((uint32_t)scheduleCount);
    fiber_detail::SharedFunc<void()> shared_func{ [atomic_fence = fiber_detail::NonMovableAtomic<size_t>(0), counter, batchCount, batch, n, begin, f = std::forward<F>(f)]() mutable {
        SKR_DEFER({ counter.decrement(); });
        size_t                       i = 0ull;
        while ((i = atomic_fence.value.fetch_add(1)) < batchCount)
        {
            auto begin_idx = i * batch;
            auto end_idx   = std::min<size_t>((i + 1) * batch, static_cast<size_t>(n));
            f(begin + begin_idx, begin + end_idx);
        }
    } };
    for (size_t i = 0; i < scheduleCount; ++i)
    {
        skr::task::schedule(shared_func, nullptr);
    }
    return counter;
}

template <class F, class Iter>
requires(std::is_invocable_v<F, Iter, Iter>)
void concurrent(Iter begin, Iter end, size_t batch, F f, size_t inplace_batch_threahold = 1ull)
{
    auto   n          = _distance(begin, end);
    size_t batchCount = (n + batch - 1) / batch;
    if (batchCount < inplace_batch_threahold)
    {
        for (size_t i = 0; i < batchCount; ++i)
        {
            auto toAdvance = std::min((size_t)n, batch);
            auto l         = begin;
            auto r         = begin;
            _advance(r, toAdvance);
            n -= toAdvance;
            f(l, r);
            begin = r;
        }
    }
    else
    {
        task::counter_t counter;
        auto            scheduleCount = std::min<size_t>(batchCount, std::thread::hardware_concurrency());
        counter.add((uint32_t)scheduleCount);
        fiber_detail::SharedFunc<void()> shared_func{ [atomic_fence = fiber_detail::NonMovableAtomic<size_t>(0), counter, batchCount, batch, n, begin, f = std::forward<F>(f)]() mutable {
            SKR_DEFER({ counter.decrement(); });
            size_t                       i = 0ull;
            while ((i = atomic_fence.value.fetch_add(1)) < batchCount)
            {
                auto begin_idx = i * batch;
                auto end_idx   = std::min<size_t>((i + 1) * batch, static_cast<size_t>(n));
                f(begin + begin_idx, begin + end_idx);
            }
        } };
        for (size_t i = 0; i < scheduleCount; ++i)
        {
            skr::task::schedule(shared_func, nullptr);
        }
        counter.wait(true);
    }
}
} // namespace skr