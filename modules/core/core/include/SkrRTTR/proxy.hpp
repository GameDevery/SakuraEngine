#pragma once

// TODO. Proxy
//   通过反射导出，以及与脚本交互
//   萃取简化
namespace skr
{
template <typename T>
struct ProxyObjectTraits {
    template <typename U>
    static constexpr bool validate()
    {
        return false;
    }
};
template <typename T, typename Proxy>
concept Proxyable = ProxyObjectTraits<Proxy>::template validate<T>();
} // namespace skr