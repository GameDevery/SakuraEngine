#pragma once
#include "SkrBase/config.h"

namespace skr::container
{
// forward declare
template <typename Func>
struct MemberFuncDelegate;
template <typename Func>
struct DelegateCoreBase;

template <typename Ret, typename... Args>
struct MemberFuncDelegate<Ret(Args...)> {
    using InvokeFunc = Ret (*)(void*, Args...);

    void*      object = nullptr;
    InvokeFunc invoke = nullptr;
};

template <typename Ret, typename... Args>
struct DelegateCoreBase<Ret(Args...)> {
    using ThisType   = DelegateCoreBase<Ret(Args...)>;
    using SizeType   = size_t;
    using InvokeFunc = Ret (*)(ThisType*, Args...);
    using DeleteFunc = void (*)(ThisType*);

    InvokeFunc invoke    = nullptr;
    DeleteFunc dtor      = nullptr;
    SizeType   ref_count = 0;
    // used for mark the kind of delegate, the meaning of this flag is defined by user
    // in skr, it is EDelegateFunctorFlag
    SizeType   ext_flag  = 0;
};

} // namespace skr::container