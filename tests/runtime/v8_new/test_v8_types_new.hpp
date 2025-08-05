#pragma once
#include "SkrCore/log.hpp"
#include <SkrRTTR/script/scriptble_object.hpp>
#ifndef __meta__
    #include "test_v8_types_new.generated.h"
#endif

// basic object
namespace test_v8
{
sreflect_struct(guid = "a1b2c3d4-e5f6-7890-abcd-ef1234567890")
sscript_visible sscript_newable
ErrorExport {
    sscript_visible
    int32_t* primitive_ref_field = {};

    sscript_visible
    skr::StringView string_view_field = {};

    sscript_visible
    ErrorExport() {}

    sscript_visible
    ErrorExport(int32_t a) {}
    
    sscript_visible
    void primitive_pointer_param(int32_t* a) {}
};

sreflect_struct(guid = "9099f452-beab-482b-a9c8-0f582bd7f5b4" rttr = @full)
sscript_visible sscript_newable
SimpleTest : public ::skr::ScriptbleObject {
    SKR_GENERATE_BODY(SimpleTest)

    sscript_visible
    static void print(skr::StringView content)
    {
        SKR_LOG_FMT_INFO(u8"{}", content.data());
    }
};
} // namespace test_v8