#pragma once
#include "SkrRTTR/type.hpp"
#include "v8-persistent-handle.h"
#include "SkrRTTR/scriptble_object.hpp"

namespace skr::v8
{
struct V8BindRecordCore {
    // native info
    skr::rttr::ScriptbleObject* object;
    skr::rttr::Type*            type;

    // v8 info
    ::v8::Persistent<::v8::Object> v8_object;
};
} // namespace skr::v8