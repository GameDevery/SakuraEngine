#pragma once
#include "SkrContainers/string.hpp"
#include <SkrBase/config.h>
#include <SkrBase/meta.h>
#ifndef __meta__
    #include "V8Playground/debug.generated.h"
#endif

namespace skr
{
// clang-format off
sreflect_struct(guid = "98fb6fdf-7223-4baf-90b0-46e746af88d2")
sscript_visible
Debug {

    sscript_visible
    static void info(StringView message);

    sscript_visible
    static void warn(StringView message);

    sscript_visible
    static void error(StringView message);

    sscript_visible
    static void exit(int32_t exit_code);

private:
    bool _shit;
};
} // namespace skr