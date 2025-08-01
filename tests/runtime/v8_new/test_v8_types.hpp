#pragma once
#include "SkrCore/log.hpp"
#include <SkrRTTR/script/scriptble_object.hpp>
#ifndef __meta__
    #include "test_v8_types.generated.h"
#endif

// basic object
namespace test_v8
{
// clang-format off
sreflect_struct(guid = "9099f452-beab-482b-a9c8-0f582bd7f5b4" rttr = @full)
sscript_visible sscript_newable
SimpleTest : public ::skr::ScriptbleObject {
    SKR_GENERATE_BODY()
    // clang-format on

    sscript_visible
    static void print()
    {
        SKR_LOG_FMT_INFO(u8"Test AAAAAAAAA");
    }
};
} // namespace test_v8