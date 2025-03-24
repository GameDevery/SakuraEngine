#pragma once
#include "SkrContainers/string.hpp"
#include <SkrBase/config.h>
#include <SkrBase/meta.h>
#ifndef __meta__
    #include "V8Playground/debug.generated.h"
#endif

namespace v8_play
{
// clang-format off
sreflect_struct(guid = "98fb6fdf-7223-4baf-90b0-46e746af88d2")
sscript_visible
Debug {
    // clang-format on

    friend struct skr::JsonSerde<Debug>;

    sscript_visible
    static void info(skr::StringView message);

    sscript_visible
    static void warn(skr::StringView message);

    sscript_visible
    static void error(skr::StringView message);

private:
    bool _shit;
};
} // namespace v8_play