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

    void*      object = nullptr;
    InvokeFunc invoke = nullptr;
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

    inline FunctorDelegateCoreNormal() {
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
    Delegate(const Delegate& other);

    // factory
    Delegate Static(FuncType* func);
    template <typename Obj>
    Delegate Member(Obj* obj, Ret (Obj::*func)(Args...));
    template <typename Functor>
    Delegate Functor(Functor&& functor);
    template <typename Functor>
    Delegate Lambda(Functor&& lambda);
    Delegate CustomFunctorCore(FunctorCore* core);

    // binder
    void bind_static(FuncType* func);
    template <typename Obj>
    void bind_member(Obj* obj, Ret (Obj::*func)(Args...));
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