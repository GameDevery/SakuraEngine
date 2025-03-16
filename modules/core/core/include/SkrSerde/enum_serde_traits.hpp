#pragma once
#include <SkrContainersDef/string.hpp>
#include <SkrContainersDef/span.hpp>
#include "SkrBase/meta.h"

// TODO. 仅供 generated.cpp 内使用，退化成静态函数
namespace skr
{
template <class T>
struct EnumSerdeTraits {
    static StringView to_string(const T& value)
    {
        static_assert(std ::is_same_v<T, T*>, "EnumTraits<T>::to_string is not implemented");
        return {};
    }
    static bool from_string(StringView str, T& value)
    {
        static_assert(std ::is_same_v<T, T*>, "EnumTraits<T>::from_string is not implemented");
        return false;
    }
};

} // namespace skr