#pragma once
#include <SkrV8/v8_fwd.hpp>
#include <SkrRTTR/script/scriptble_object.hpp>
#include <SkrRTTR/script/stack_proxy.hpp>

// v8 includes
#include <v8-isolate.h>
#include <v8-platform.h>
#include <v8-primitive.h>

namespace skr
{
struct SKR_V8_API V8Value {
    // ctor & dtor
    V8Value();
    V8Value(v8::Global<v8::Value> v8_value, V8Context* context);
    ~V8Value();

    // ops
    inline bool is_empty() const
    {
        return _v8_value.IsEmpty();
    }
    inline void reset()
    {
        _v8_value.Reset();
        _context = nullptr;
    }

    // get kind
    bool is_object() const;
    bool is_function() const;

    // is & as
    template <typename T>
    inline bool is() const
    {
        if constexpr (std::is_pointer_v<T>)
        {
            using RawType = std::remove_pointer_t<T>;

            if constexpr (std::derived_from<RawType, ScriptbleObject>)
            { // object case
                TypeSignatureTyped<RawType> sig;
                return _is(sig);
            }
            else
            {
                return false;
            }
        }
        else
        {
            TypeSignatureTyped<T> sig;
            return _is(sig.view());
        }
    }
    template <typename T>
    inline Optional<T> as() const
    {
        if (is<T>())
        {
            Placeholder<T> result_holder;
            if constexpr (std::is_pointer_v<T>)
            {
                using RawType = std::remove_pointer_t<T>;

                if constexpr (std::derived_from<RawType, ScriptbleObject>)
                { // object case
                    TypeSignatureTyped<RawType> sig;
                    return _as(sig, result_holder.data());
                }
                else
                {
                    SKR_UNREACHABLE_CODE();
                }
            }
            else
            {
                TypeSignatureTyped<T> sig;
                return _as(sig.view(), result_holder.data());
            }
            return { std::move(*result_holder.data_typed()) };
        }
        else
        {
            return {};
        }
    }

    // get field
    V8Value get(StringView name) const;

    // invoke
    template <typename Ret, typename... Args>
    inline decltype(auto) call(Args&&... args) const
    {
        if constexpr (std::is_same_v<Ret, void>)
        {
            if (!is_function()) { return false; }
            return _call(
                { StackProxyMaker<Args>::Make(std::forward<Args>(args))... },
                {}
            );
        }
        else
        {
            if (!is_function()) { return Optional<Ret>{}; }
            return _call(
                { StackProxyMaker<Args>::Make(std::forward<Args>(args))... },
                { .data = Placeholder<Ret>::make(), .signature = type_signature_of<Ret>() }
            );
        }
    }
    template <typename Ret, typename... Args>
    inline decltype(auto) call_method(const StringView name, Args&&... args) const
    {
        if constexpr (std::is_same_v<Ret, void>)
        {
            if (!is_object()) { return false; }
            return _call_method(
                name,
                { StackProxyMaker<Args>::Make(std::forward<Args>(args))... },
                {}
            );
        }
        else
        {
            if (!is_object()) { return Optional<Ret>{}; }
            return _call_method(
                name,
                { StackProxyMaker<Args>::Make(std::forward<Args>(args))... },
                { .data = Placeholder<Ret>::make(), .signature = type_signature_of<Ret>() }
            );
        }
    }

private:
    void _as(TypeSignatureView sig, void* ptr) const;
    bool _is(TypeSignatureView sig) const;
    bool _call(
        const span<const StackProxy> params,
        StackProxy                   return_value
    ) const;
    bool _call_method(
        const StringView             name,
        const span<const StackProxy> params,
        StackProxy                   return_value
    ) const;

private:
    v8::Global<v8::Value> _v8_value = {};
    V8Context*            _context  = nullptr;
};
} // namespace skr