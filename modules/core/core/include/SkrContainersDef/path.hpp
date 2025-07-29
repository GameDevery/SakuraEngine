#pragma once
#include "SkrBase/containers/path/path.hpp"
#include "SkrContainersDef/string.hpp"

namespace skr
{
    // Import path class to skr namespace
    using Path = ::skr::Path;
    
    // Additional helper functions
    inline Path operator/(const char8_t* lhs, const Path& rhs) noexcept
    {
        return Path(lhs) / rhs;
    }
    
    inline Path operator/(const String& lhs, const Path& rhs) noexcept
    {
        return Path(lhs) / rhs;
    }
    
    inline Path operator/(StringView lhs, const Path& rhs) noexcept
    {
        return Path(lhs) / rhs;
    }
}