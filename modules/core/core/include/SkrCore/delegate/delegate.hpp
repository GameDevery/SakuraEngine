#pragma once
#include "SkrBase/config.h"
#include <type_traits>

namespace skr
{
template <typename Func>
struct Delegate;
template <typename Func>
struct MemberFuncDelegateCore;
template <typename Func>
struct FunctorDelegateCore;
template <typename Func, typename Functor>
struct FunctorDelegateCoreNormal;

enum class EDelegateKind : uint8_t
{
    Empty,
    Static,
    Member,
    Functor,
};

template <typename Ret, typename... Args>
struct MemberFuncDelegateCore<Ret(Args...)> {
    using InvokeFunc = Ret (*)(void*, Args...);
    using ThisType  = MemberFuncDelegateCore<Ret(Args...)>;

    void*      object = nullptr;
    InvokeFunc invoke = nullptr;

    template<typename Obj, Ret (Obj::*Func)(Args...)>
    static ThisType Make(Obj* obj)
    {
        ThisType core;
        core.object = obj;
        core.invoke = [](void* obj, Args... args) -> Ret {
            return (static_cast<Obj*>(obj)->*Func)(std::forward<Args>(args)...);
        };
        return core;
    }
};
template <typename Ret, typename... Args>
struct FunctorDelegateCore<Ret(Args...)> {
    using ThisType   = FunctorDelegateCore<Ret(Args...)>;
    using SizeType   = size_t;
    using InvokeFunc = Ret (*)(ThisType*, Args...);
    using DeleteFunc = void (*)(ThisType*);

    InvokeFunc invoke    = nullptr;
    DeleteFunc dtor      = nullptr;
    SizeType   ref_count = 0;
};
template <typename Functor, typename Ret, typename... Args>
struct FunctorDelegateCoreNormal<Ret(Args...), Functor> : public FunctorDelegateCore<Ret(Args...)> {
    using Super = FunctorDelegateCore<Ret(Args...)>;

    Functor functor;

    inline FunctorDelegateCoreNormal()
    {
        Super::invoke = [](Super* core, Args... args) -> Ret {
            return static_cast<FunctorDelegateCoreNormal*>(core)->functor(std::forward<Args>(args)...);
        };
        Super::dtor = [](Super* core) {
            ~FunctorDelegateCoreNormal();
        };
    }
};

template <typename Ret, typename... Args>
struct Delegate<Ret(Args...)> {
    using FuncType    = Ret(Args...);
    using MemberCore  = MemberFuncDelegateCore<FuncType>;
    using FunctorCore = FunctorDelegateCore<FuncType>;

    // ctor & dtor
    Delegate();
    ~Delegate();

    // copy & move
    Delegate(const Delegate& other);
    Delegate(Delegate&& other);

    // assign & move assign
    Delegate& operator=(const Delegate& other);
    Delegate& operator=(Delegate&& other);

    // factory
    Delegate Static(FuncType* func);
    template <typename Obj, Ret (Obj::*Func)(Args...)>
    Delegate Member(Obj* obj);
    template <typename Functor>
    Delegate Functor(Functor&& functor);
    template <typename Functor>
    Delegate Lambda(Functor&& lambda);
    Delegate CustomFunctorCore(FunctorCore* core);

    // binder
    void bind_static(FuncType* func);
    template <typename Obj, Ret (Obj::*Func)(Args...)>
    void bind_member(Obj* obj);
    template <typename Functor>
    void bind_functor(Functor&& functor);
    template <typename Functor>
    void bind_lambda(Functor&& lambda);
    void bind_custom_functor_core(FunctorCore* core);

    // invoke
    Ret invoke(Args... args);

    // ops
    void reset();

    // getter
    EDelegateKind kind() const;
    bool          is_empty() const;

private:
    union
    {
        FuncType*    _static_delegate;
        MemberCore   _member_delegate;
        FunctorCore* _functor_delegate;
    };
    EDelegateKind _kind = EDelegateKind::Empty;
};
} // namespace skr

namespace skr
{
// ctor & dtor
template <typename Ret, typename... Args>
inline Delegate<Ret(Args...)>::Delegate()
{
}
template <typename Ret, typename... Args>
inline Delegate<Ret(Args...)>::~Delegate()
{
    reset();
}

// copy & move
template <typename Ret, typename... Args>
inline Delegate<Ret(Args...)>::Delegate(const Delegate& other)
{
    _kind = other._kind;
    switch (_kind)
    {
    case EDelegateKind::Empty:
        break;
    case EDelegateKind::Static:
        _static_delegate = other._static_delegate;
        break;
    case EDelegateKind::Member:
        _member_delegate = other._member_delegate;
        break;
    case EDelegateKind::Functor:
        _functor_delegate = other._functor_delegate;
        _functor_delegate->ref_count++;
        break;
    }
}
template <typename Ret, typename... Args>
inline Delegate<Ret(Args...)>::Delegate(Delegate&& other)
{
    _kind = other._kind;
    switch (_kind)
    {
    case EDelegateKind::Empty:
        break;
    case EDelegateKind::Static:
        _static_delegate = other._static_delegate;
        break;
    case EDelegateKind::Member:
        _member_delegate = other._member_delegate;
        break;
    case EDelegateKind::Functor:
        _functor_delegate = other._functor_delegate;
        other._functor_delegate = nullptr;
        break;
    }
    other._kind = EDelegateKind::Empty;
}

// assign & move assign
template <typename Ret, typename... Args>
inline Delegate<Ret(Args...)>& Delegate<Ret(Args...)>::operator=(const Delegate& other)
{
    reset();
    _kind = other._kind;
    switch (_kind)
    {
    case EDelegateKind::Empty:
        break;
    case EDelegateKind::Static:
        _static_delegate = other._static_delegate;
        break;
    case EDelegateKind::Member:
        _member_delegate = other._member_delegate;
        break;
    case EDelegateKind::Functor:    
        _functor_delegate = other._functor_delegate;
        _functor_delegate->ref_count++;
        break;
    }
}
template <typename Ret, typename... Args>
inline Delegate<Ret(Args...)>& Delegate<Ret(Args...)>::operator=(Delegate&& other)
{
    reset();
    _kind = other._kind;
    switch (_kind)
    {
    case EDelegateKind::Empty:
        break;
    case EDelegateKind::Static:
        _static_delegate = other._static_delegate;
        break;
    case EDelegateKind::Member:
        _member_delegate = other._member_delegate;
        break;
    case EDelegateKind::Functor:
        _functor_delegate = other._functor_delegate;
        other._functor_delegate = nullptr;
        break;
    }
    other._kind = EDelegateKind::Empty;
}

// factory
template <typename Ret, typename... Args>
inline Delegate<Ret(Args...)> Delegate<Ret(Args...)>::Static(FuncType* func)
{
    Delegate delegate;
    delegate.bind_static(func);
    return delegate;
}
template <typename Ret, typename... Args>
template <typename Obj, Ret (Obj::*Func)(Args...)>
inline Delegate<Ret(Args...)> Delegate<Ret(Args...)>::Member(Obj* obj)
{
    Delegate delegate;
    delegate.bind_member<Obj, Func>(obj);
    return delegate;
}
template <typename Ret, typename... Args>
template <typename Functor>
inline Delegate<Ret(Args...)> Delegate<Ret(Args...)>::Functor(Functor&& functor)
{
    Delegate delegate;
    delegate.bind_functor(std::forward<Functor>(functor));
    return delegate;
}
template <typename Ret, typename... Args>
template <typename Functor>
inline Delegate<Ret(Args...)> Delegate<Ret(Args...)>::Lambda(Functor&& lambda)
{
    Delegate delegate;
    delegate.bind_lambda(std::forward<Functor>(lambda));
    return delegate;
}
template <typename Ret, typename... Args>
inline Delegate<Ret(Args...)> Delegate<Ret(Args...)>::CustomFunctorCore(FunctorCore* core)
{
    Delegate delegate;
    delegate.bind_custom_functor_core(core);
    return delegate;
}

// binder
template <typename Ret, typename... Args>
inline void Delegate<Ret(Args...)>::bind_static(FuncType* func)
{
    reset();
    _static_delegate = func;
    _kind           = EDelegateKind::Static;
}
template <typename Ret, typename... Args>
template <typename Obj, Ret (Obj::*Func)(Args...)>
inline void Delegate<Ret(Args...)>::bind_member(Obj* obj)
{
    reset();
    _member_delegate = MemberCore::template Make<Obj, Func>(obj);
    _kind            = EDelegateKind::Member;
}
template <typename Ret, typename... Args>
template <typename Functor>
inline void Delegate<Ret(Args...)>::bind_functor(Functor&& functor)
{
    reset();
    _functor_delegate = SkrNew<FunctorDelegateCoreNormal<FuncType, Functor>>();
    _functor_delegate->functor = std::forward<Functor>(functor);
    _functor_delegate->ref_count++;
    _kind = EDelegateKind::Functor;
}
template <typename Ret, typename... Args>
template <typename Functor>
inline void Delegate<Ret(Args...)>::bind_lambda(Functor&& lambda)
{
    reset();
    _functor_delegate = SkrNew<FunctorDelegateCoreNormal<FuncType, Functor>>();
    _functor_delegate->functor = std::forward<Functor>(lambda);
    _functor_delegate->ref_count++;
    _kind = EDelegateKind::Functor;
}
template <typename Ret, typename... Args>
inline void Delegate<Ret(Args...)>::bind_custom_functor_core(FunctorCore* core)
{
    reset();
    _functor_delegate = core;
    _functor_delegate->ref_count++;
    _kind = EDelegateKind::Functor;
}
}