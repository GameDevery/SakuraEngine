#pragma once
#include "SkrCore/memory/memory.h"
#include "SkrContainersDef/string.hpp"
#include "SkrContainersDef/stl_function.hpp"
#include "SkrContainersDef/tuple.hpp"

namespace skr
{
namespace logging
{

struct LogFormatter;

struct ArgsList {
    template <typename... Args>
    void push(Args&&... args) SKR_NOEXCEPT;

protected:
    friend struct LogFormatter;
    skr::stl_function<bool(const skr::String& format, LogFormatter&)> format_;
};

struct LogFormatter {
    ~LogFormatter() SKR_NOEXCEPT;

    [[nodiscard]] skr::String const& format(
    const skr::String& format,
    const ArgsList&    args_list);

    skr::String formatted_string = u8"";
};

template <typename... Args>
SKR_FORCEINLINE void ArgsList::push(Args&&... args) SKR_NOEXCEPT
{
    format_ = [args = std::make_tuple(std::forward<Args>(args)...)](const skr::String& format, LogFormatter& formatter) mutable {
        return std::apply([&](auto&&... args) {
            formatter.formatted_string = skr::format(format, std::forward<Args>(args)...);
            return true;
        },
                          std::move(args));
    };
}

} // namespace logging
} // namespace skr