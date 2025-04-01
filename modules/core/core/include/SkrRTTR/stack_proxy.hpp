#pragma once
#include <SkrRTTR/type_signature.hpp>

namespace skr
{
struct StackProxy {
    void*         data      = nullptr;
    TypeSignature signature = {};
};
template <typename T>
struct StackProxyMaker {
    inline static StackProxy Make(T& data)
    {
        return {
            .data      = &data,
            .signature = type_signature_of<T>(),
        };
    }
    inline static StackProxy Make(T&& data)
    {
        return {
            .data      = &data,
            .signature = type_signature_of<T>(),
        };
    }
};
template <typename T>
struct StackProxyMaker<T*> {
    inline static StackProxy Make(T* data)
    {
        return {
            .data      = data,
            .signature = type_signature_of<T*>(),
        };
    }
};
template <typename T>
struct StackProxyMaker<const T*> {
    inline static StackProxy Make(const T* data)
    {
        return {
            .data      = const_cast<T*>(data),
            .signature = type_signature_of<T*>(),
        };
    }
};
template <typename T>
struct StackProxyMaker<T&> {
    inline static StackProxy Make(T& data)
    {
        return {
            .data      = &data,
            .signature = type_signature_of<T&>(),
        };
    }
};
template <typename T>
struct StackProxyMaker<const T&> {
    inline static StackProxy Make(const T& data)
    {
        return {
            .data      = const_cast<T*>(&data),
            .signature = type_signature_of<const T&>(),
        };
    }
};
template <typename T>
struct StackProxyMaker<T&&> {
    inline static StackProxy Make(T&& data)
    {
        return {
            .data      = &data,
            .signature = type_signature_of<T&&>(),
        };
    }
};

} // namespace skr