#pragma once
#include <SkrBase/containers/delegate/delegate_core.hpp>

// TODO. delegate 的实现放在 core 内比较合适，同时也方便与 core 提供的机制联动
namespace skr::container
{
template <typename Func, typename Allocator>
struct Delegate;

template <typename Ret, typename... Args, typename Allocator>
struct Delegate<Ret(Args...), Allocator> : Allocator {
    using FuncType = Ret(Args...);

    enum class EKind : uint8_t
    {
        Empty,
        Static,
        Member,
        Functor,
    };

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

    // invoke
    Ret invoke(Args... args);

    // getter
    EKind kind() const;
    bool  is_empty() const;

private:
    union
    {
        FuncType*                    _static_delegate;
        MemberFuncDelegate<FuncType> _member_delegate;
        DelegateCoreBase<FuncType>*  _functor_delegate;
    };
    EKind _kind = EKind::Empty;
};
} // namespace skr::container