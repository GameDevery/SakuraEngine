#pragma once
#include <SkrCore/delegate/delegate.hpp>
#include <SkrContainers/sparse_vector.hpp>

namespace skr
{
template <typename Func>
struct Event;
using EventBindID = uint64_t;

template <typename... Args>
struct Event<void(Args...)> {
    using FuncType    = void(Args...);
    using MemberCore  = MemberFuncDelegateCore<FuncType>;
    using FunctorCore = FunctorDelegateCore<FuncType>;

    // ctor & dtor
    Event();
    ~Event();

    // copy & move
    Event(const Event& other);
    Event(Event&& other);

    // assign & move assign
    Event& operator=(const Event& other);
    Event& operator=(Event&& other);

    // binder
    EventBindID bind_static(FuncType* func);
    template <auto MemberFunc>
    EventBindID bind_member(typename MemberFuncTraits<decltype(MemberFunc)>::ObjPtrType obj);
    template <typename Functor>
    EventBindID bind_functor(Functor&& functor);
    template <typename Functor>
    EventBindID bind_lambda(Functor&& lambda);
    EventBindID bind_custom_functor_core(FunctorCore* core);

    // remove
    void remove(EventBindID id);

    // invoke
    void invoke(Args... args);

    // ops
    void reset();

    // getter
    bool is_empty() const;

private:
    SparseVector<Delegate<FuncType>> _delegates;
};
} // namespace skr

namespace skr
{
// ctor & dtor
template <typename... Args>
inline Event<void(Args...)>::Event() = default;
template <typename... Args>
inline Event<void(Args...)>::~Event() = default;

// copy & move
template <typename... Args>
inline Event<void(Args...)>::Event(const Event& other) = default;
template <typename... Args>
inline Event<void(Args...)>::Event(Event&& other) = default;

// assign & move assign
template <typename... Args>
inline Event<void(Args...)>& Event<void(Args...)>::operator=(const Event& other) = default;
template <typename... Args>
inline Event<void(Args...)>& Event<void(Args...)>::operator=(Event&& other) = default;

// binder
template <typename... Args>
inline EventBindID Event<void(Args...)>::bind_static(FuncType* func)
{
    auto result = _delegates.add_default();
    result.ref().template bind_static(func);
    return result.index();
}
template <typename... Args>
template <auto MemberFunc>
inline EventBindID Event<void(Args...)>::bind_member(typename MemberFuncTraits<decltype(MemberFunc)>::ObjPtrType obj)
{
    auto result = _delegates.add_default();
    result.ref().template bind_member<MemberFunc>(obj);
    return result.index();
}
template <typename... Args>
template <typename Functor>
inline EventBindID Event<void(Args...)>::bind_functor(Functor&& functor)
{
    auto result = _delegates.add_default();
    result.ref().template bind_functor(std::forward<Functor>(functor));
    return result.index();
}
template <typename... Args>
template <typename Functor>
inline EventBindID Event<void(Args...)>::bind_lambda(Functor&& lambda)
{
    auto result = _delegates.add_default();
    result.ref().template bind_lambda(std::forward<Functor>(lambda));
    return result.index();
}
template <typename... Args>
inline EventBindID Event<void(Args...)>::bind_custom_functor_core(FunctorCore* core)
{
    auto result = _delegates.add_default();
    result.ref().bind_custom_functor_core(core);
    return result.index();
}

// remove
template <typename... Args>
inline void Event<void(Args...)>::remove(EventBindID id)
{
    _delegates.remove_at(id);
}

// invoke
template <typename... Args>
inline void Event<void(Args...)>::invoke(Args... args)
{
    for (auto& delegate : _delegates)
    {
        delegate.invoke(std::forward<Args>(args)...);
    }
}

// ops
template <typename... Args>
inline void Event<void(Args...)>::reset()
{
    _delegates.clear();
}

// getter
template <typename... Args>
inline bool Event<void(Args...)>::is_empty() const
{
    return _delegates.is_empty();
}

} // namespace skr