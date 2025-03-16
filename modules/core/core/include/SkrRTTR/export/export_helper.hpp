#pragma once
#include <type_traits>
#include "SkrRTTR/export/export_data.hpp"
#include "SkrRTTR/export/dynamic_stack.hpp"

namespace skr
{
struct RTTRExportHelper {
    // ctor & dtor export
    template <typename T, typename... Args>
    inline static void* export_ctor()
    {
        auto result = +[](void* p, Args... args) {
            new (p) T(std::forward<Args>(args)...);
        };
        return reinterpret_cast<void*>(result);
    }
    template <typename T, typename... Args>
    inline static MethodInvokerDynamicStack export_ctor_dynamic_stack()
    {
        return _make_ctor_dynamic_stack<T, Args...>(std::make_index_sequence<sizeof...(Args)>());
    }
    template <typename T>
    inline static DtorInvoker export_dtor()
    {
        auto result = +[](void* p) {
            reinterpret_cast<T*>(p)->~T();
        };
        return result;
    }

    // method export
    template <auto method>
    inline static void* export_method()
    {
        return _make_method_proxy<method>(method);
    }
    template <auto method>
    inline static MethodInvokerDynamicStack export_method_dynamic_stack()
    {
        return _make_method_dynamic_stack<method>(method);
    }
    template <auto method>
    inline static void* export_static_method()
    {
        return reinterpret_cast<void*>(method);
    }
    template <auto method>
    inline static FuncInvokerDynamicStack export_static_method_dynamic_stack()
    {
        return _make_function_dynamic_stack<method>(method);
    }

    // extern method export
    template <auto method>
    inline static void* export_extern_method()
    {
        return reinterpret_cast<void*>(method);
    }
    template <auto method>
    inline static FuncInvokerDynamicStack export_extern_method_dynamic_stack()
    {
        return _make_function_dynamic_stack<method>(method);
    }

    // function export
    template <auto func>
    inline static void* export_function()
    {
        return reinterpret_cast<void*>(func);
    }
    template <auto method>
    inline static FuncInvokerDynamicStack export_function_dynamic_stack()
    {
        return _make_function_dynamic_stack<method>(method);
    }

    // invoker
    template <typename... Args>
    using CtorInvoker = void (*)(void*, Args...);
    using DtorInvoker = void (*)(void*);
    template <typename Ret, typename... Args>
    using MethodInvokerExpand = Ret (*)(void*, Args...);
    template <typename Ret, typename... Args>
    using ConstMethodInvokerExpand = Ret (*)(const void*, Args...);
    template <typename Ret, typename... Args>
    using FunctionInvokerExpand = Ret (*)(Args...);
    template <typename Func>
    struct ExpandInvoker {
    };
    template <typename Ret, typename... Args>
    struct ExpandInvoker<Ret (*)(Args...)> {
        using MethodInvoker      = MethodInvokerExpand<Ret, Args...>;
        using ConstMethodInvoker = ConstMethodInvokerExpand<Ret, Args...>;
        using FunctionInvoker    = FunctionInvokerExpand<Ret, Args...>;
    };
    template <typename Func>
    using MethodInvoker = typename ExpandInvoker<Func>::MethodInvoker;
    template <typename Func>
    using ConstMethodInvoker = typename ExpandInvoker<Func>::ConstMethodInvoker;
    template <typename Func>
    using FunctionInvoker = typename ExpandInvoker<Func>::FunctionInvoker;

private:
    // member proxy helpers
    template <auto method, class T, typename Ret, typename... Args>
    inline static void* _make_method_proxy(Ret (T::*)(Args...))
    {
        auto proxy = +[](void* obj, Args... args) -> Ret {
            return (reinterpret_cast<T*>(obj)->*method)(std::forward<Args>(args)...);
        };
        return reinterpret_cast<void*>(proxy);
    }
    template <auto method, class T, typename Ret, typename... Args>
    inline static void* _make_method_proxy(Ret (T::*)(Args...) const)
    {
        auto proxy = +[](const void* obj, Args... args) -> Ret {
            return (reinterpret_cast<const T*>(obj)->*method)(std::forward<Args>(args)...);
        };
        return reinterpret_cast<void*>(proxy);
    }

    // dynamic stack helpers
    template <typename T, typename... Args, size_t... Idx>
    inline static MethodInvokerDynamicStack _make_ctor_dynamic_stack(std::index_sequence<Idx...>)
    {
        return +[](void* p, DynamicStack& stack) {
            new (p) T(std::forward<Args>(stack.get_param<Args>(Idx))...);
        };
    }
    template <auto method, typename T, typename Ret, typename... Args>
    inline static MethodInvokerDynamicStack _make_method_dynamic_stack(Ret (T::*)(Args...))
    {
        return _make_method_dynamic_stack_helper<method, T, Ret, Args...>(std::make_index_sequence<sizeof...(Args)>());
    }
    template <auto method, typename T, typename Ret, typename... Args>
    inline static MethodInvokerDynamicStack _make_method_dynamic_stack(Ret (T::*)(Args...) const)
    {
        return _make_method_dynamic_stack_helper_const<method, T, Ret, Args...>(std::make_index_sequence<sizeof...(Args)>());
    }
    template <auto method, typename T, typename Ret, typename... Args, size_t... Idx>
    inline static MethodInvokerDynamicStack _make_method_dynamic_stack_helper(std::index_sequence<Idx...>)
    {
        return +[](void* p, DynamicStack& stack) {
            if constexpr (std::is_same_v<void, Ret>)
            {
                (reinterpret_cast<T*>(p)->*method)(std::forward<Args>(stack.get_param<Args>(Idx))...);
            }
            else
            {
                if (stack.need_store_return())
                {
                    stack.store_return<Ret>(
                        (reinterpret_cast<T*>(p)->*method)(std::forward<Args>(stack.get_param<Args>(Idx))...)
                    );
                }
                else
                {
                    (reinterpret_cast<T*>(p)->*method)(std::forward<Args>(stack.get_param<Args>(Idx))...);
                }
            }
        };
    }
    template <auto method, typename T, typename Ret, typename... Args, size_t... Idx>
    inline static MethodInvokerDynamicStack _make_method_dynamic_stack_helper_const(std::index_sequence<Idx...>)
    {
        return +[](void* p, DynamicStack& stack) {
            if constexpr (std::is_same_v<void, Ret>)
            {
                (reinterpret_cast<const T*>(p)->*method)(std::forward<Args>(stack.get_param<Args>(Idx))...);
            }
            else
            {
                if (stack.need_store_return())
                {
                    stack.store_return<Ret>(
                        (reinterpret_cast<T*>(p)->*method)(std::forward<Args>(stack.get_param<Args>(Idx))...)
                    );
                }
                else
                {
                    (reinterpret_cast<const T*>(p)->*method)(std::forward<Args>(stack.get_param<Args>(Idx))...);
                }
            }
        };
    }
    template <auto func, typename Ret, typename... Args>
    inline static FuncInvokerDynamicStack _make_function_dynamic_stack(Ret (*)(Args...))
    {
        return _make_function_dynamic_stack_helper<func, Ret, Args...>(std::make_index_sequence<sizeof...(Args)>());
    }
    template <auto func, typename Ret, typename... Args, size_t... Idx>
    inline static FuncInvokerDynamicStack _make_function_dynamic_stack_helper(std::index_sequence<Idx...>)
    {
        return +[](DynamicStack& stack) {
            if constexpr (std::is_same_v<void, Ret>)
            {
                func(std::forward<Args>(stack.get_param<Args>(Idx))...);
            }
            else
            {
                if (stack.need_store_return())
                {
                    stack.store_return<Ret>(
                        func(std::forward<Args>(stack.get_param<Args>(Idx))...)
                    );
                }
                else
                {
                    func(std::forward<Args>(stack.get_param<Args>(Idx))...);
                }
            }
        };
    }
};
} // namespace skr
