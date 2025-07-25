#pragma once
#include "SkrBase/config.h" // IWYU pragma: keep

#if __cpp_impl_coroutine
#include <atomic>
#include "SkrOS/thread.h"
#include "SkrContainers/array.hpp"
#include "SkrContainers/vector.hpp"
#include "SkrContainers/stl_function.hpp"
#include "SkrCore/memory/sp.hpp"

#if __cpp_lib_coroutine
    #include <coroutine>
#else
    #include <experimental/coroutine>
    //UB
    namespace std
    {
        template<class T>
        using coroutine_handle = experimental::coroutine_handle<T>;
        using suspend_always = experimental::suspend_always;
        using suspend_never = experimental::suspend_never;
    }
#endif

#include "SkrProfile/profile.h"

namespace skr
{
namespace task2
{
    template<class T>
    using state_ptr_t = SP<T>;
    template<class T>
    using state_weak_ptr_t = SPWeak<T>;
    struct SKR_TASK_API scheudler_config_t
    {
        scheudler_config_t();
        bool setAffinity = true;
        uint32_t numThreads = 0;
    };
#ifdef SKR_PROFILE_ENABLE
    struct skr_task_name_t
    {
        const char* name;
    };
    #define SkrProfileTask(name) SkrFiberEnter(name); co_yield skr_task_name_t{name}
#else
    #define SkrProfileTask(name)
#endif
    struct skr_task_t
    {
        struct promise_type
        {
            SKR_TASK_API skr_task_t get_return_object();
            std::suspend_always initial_suspend() { return {}; }
#ifdef SKR_PROFILE_ENABLE
            std::suspend_never final_suspend() noexcept
            {
                if(name != nullptr)
                    SkrFiberLeave;
                return {};
            }
#else
            std::suspend_never final_suspend() noexcept { return {}; }
#endif
            void return_void() {}
            SKR_TASK_API void unhandled_exception();
#ifdef SKR_PROFILE_ENABLE
            std::suspend_never yield_value(skr_task_name_t tn) { name = tn.name; return {}; }
#endif

            void* operator new(size_t size) { return sakura_malloc(size); }
            void operator delete(void* ptr, size_t size) { sakura_free(ptr); }
#ifdef SKR_PROFILE_ENABLE
            const char* name = nullptr;
#endif
        };
        skr_task_t(std::coroutine_handle<promise_type> coroutine) : coroutine(coroutine) {}
        skr_task_t(skr_task_t&& other) : coroutine(other.coroutine) { other.coroutine = nullptr; }
        skr_task_t() = default;
        skr_task_t& operator=(skr_task_t&& other) { coroutine = other.coroutine; other.coroutine = nullptr; return *this; }
        ~skr_task_t() { if(coroutine) coroutine.destroy(); }
        void resume() { coroutine.resume(); }
        bool done() const { return coroutine.done(); }
        explicit operator bool() const { return (bool)coroutine; }
        std::coroutine_handle<promise_type> coroutine;
    };

    struct condvar_t
    {
        SConditionVariable cv;
        skr::Vector<std::coroutine_handle<skr_task_t::promise_type>> waiters;
        skr::Vector<int> workerIndices;
        std::atomic<int> numWaiting = {0};
        std::atomic<int> numWaitingOnCondition = {0};
        condvar_t()
        {
            skr_init_condition_var(&cv);
        }
        ~condvar_t()
        {
            skr_destroy_condition_var(&cv);
        }
        void notify();
        void add_waiter(std::coroutine_handle<skr_task_t::promise_type> waiter, int workerIndex);
        void wait(SMutex& mutex);
    };
    struct event_t
    {
        struct State
        {
            condvar_t cv;
            SMutex mutex;
            bool signalled = false;
            State()
            {
                skr_init_mutex(&mutex);
            }
            ~State()
            {
                skr_destroy_mutex(&mutex);
            }
        };

        event_t()
            : state(SkrNew<State>())
        {
        }
        event_t(const event_t& other) : state(other.state) {}
        event_t(event_t&& other) : state(std::move(other.state)) {}
        event_t& operator=(const event_t& other) { state = other.state; return *this;}
        event_t& operator=(event_t&& other) { state = std::move(other.state); return *this;}
        event_t(std::nullptr_t)
            : state(nullptr)
        {
        }
        ~event_t()
        {
        }
        SKR_TASK_API void notify();
        SKR_TASK_API void reset();
        size_t hash() const { return (size_t)state.get(); }
        bool done() const 
        { 
            if(!state) 
                return true;
            SMutexLock guard(state->mutex);
            return state->signalled; 
        }
        explicit operator bool() const { return (bool)state; }
        bool operator==(const event_t& other) const { return state == other.state; }

        event_t(state_ptr_t<State> state) : state(std::move(state)) {}
        state_ptr_t<State> state;
    };
    struct weak_event_t
    {
        weak_event_t(event_t& e) : state(e.state) {}
        weak_event_t(const weak_event_t& other) = default;
        weak_event_t(std::nullptr_t) {}
        ~weak_event_t() = default;
        event_t lock() const { return event_t{ state.lock() }; }
        bool expired() const { return state.is_expired(); }

        state_weak_ptr_t<event_t::State> state;
    };
    struct counter_t
    {
        struct State
        {
            SMutex mutex;
            std::atomic<uint32_t> count = 0;
            bool inverse = false;
            condvar_t cv;
            State()
            {
                skr_init_mutex(&mutex);
            }
            ~State()
            {
                skr_destroy_mutex(&mutex);
            }
        };

        counter_t()
            : state(SkrNew<State>())
        {
        }
        counter_t(bool inverse)
            : state(SkrNew<State>())
        {
            state->inverse = inverse;
        }
        counter_t(std::nullptr_t)
            : state(nullptr)
        {
        }
        counter_t(const counter_t& other) : state(other.state) {}
        counter_t(counter_t&& other) : state(std::move(other.state)) {}
        counter_t& operator=(const counter_t& other) { state = other.state; return *this;}
        counter_t& operator=(counter_t&& other) { state = std::move(other.state); return *this;}
        ~counter_t()
        {
        }
        SKR_TASK_API void add(uint32_t count);
        size_t hash() const { return (size_t)state.get(); }
        SKR_TASK_API bool decrease();
        bool done() const 
        { 
            if(!state) 
                return true;
            return (state->count == 0) == (!state->inverse); 
        }
        explicit operator bool() const { return (bool)state; }
        bool operator==(const counter_t& other) const { return state == other.state; }

        counter_t(state_ptr_t<State> state) : state(std::move(state)) {}
        state_ptr_t<State> state;
    };

    struct weak_counter_t
    {
        weak_counter_t(counter_t& e) : state(e.state) {}
        weak_counter_t(const weak_counter_t& other) = default;
        weak_counter_t(std::nullptr_t) {}
        ~weak_counter_t() = default;
        counter_t lock() const { return counter_t{ state.lock() }; }
        bool expired() const { return state.is_expired(); }

        state_weak_ptr_t<counter_t::State> state;
    };

    struct SKR_TASK_API scheduler_t
    {
        void initialize(const scheudler_config_t&);
        void bind();
        void unbind();
        void shutdown();
        static scheduler_t* instance();
        void schedule(skr_task_t&& task);
        void schedule(skr::stl_function<void()>&& function);
        struct SKR_TASK_API EventAwaitable
        {
            EventAwaitable(scheduler_t& s, event_t event, int workerIdx = -1);
            bool await_ready() const;
            bool await_suspend(std::coroutine_handle<skr_task_t::promise_type>);
            void await_resume() const {}
            scheduler_t& scheduler;
            event_t event;
            int workerIdx = -1;
        };
        struct SKR_TASK_API CounterAwaitable
        {
            CounterAwaitable(scheduler_t& s, counter_t counter, int workerIdx = -1);
            bool await_ready() const;
            bool await_suspend(std::coroutine_handle<skr_task_t::promise_type>);
            void await_resume() const {}
            scheduler_t& scheduler;
            counter_t counter;
            int workerIdx = -1;
        };
        void sync(event_t event);
        void sync(counter_t counter);
        skr::Array<std::atomic<int>, 8> spinningWorkers;
        std::atomic<unsigned int> nextSpinningWorkerIdx = {0x8000000};
        std::atomic<unsigned int> nextEnqueueIndex = {0};
        skr::Array<void*, 256> workers;
        void* mainWorker = nullptr;
        scheudler_config_t config;
        bool initialized = false;
        bool binded = false;
    };

    inline void schedule(skr_task_t&& task)
    {
        scheduler_t::instance()->schedule(std::move(task));
    }
    inline void schedule(skr::stl_function<void()>&& function)
    {
        scheduler_t::instance()->schedule(std::move(function));
    }
    SKR_TASK_API scheduler_t::EventAwaitable co_wait(event_t event, bool pinned = false);
    SKR_TASK_API scheduler_t::CounterAwaitable co_wait(counter_t counter, bool pinned = false);
    SKR_TASK_API void wait(event_t event);
    SKR_TASK_API void wait(counter_t counter);
    inline void sync(event_t event)
    {
        scheduler_t::instance()->sync(std::move(event));
    }
    inline void sync(counter_t counter)
    {
        scheduler_t::instance()->sync(std::move(counter));
    }
}
}
#endif