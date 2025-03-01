#pragma once
#include "SkrRTTR/type.hpp"
#include "v8-persistent-handle.h"

namespace skr::v8
{
struct V8RecordBindData {
    void*            data;
    skr::rttr::Type* type;

    // TODO. ctor data
    // TODO. method data
    // TODO. static method data
    // TODO. extern method data
};

enum class EV8RecordOwner
{
    CPP,
    V8
};
struct V8BindRecordCore {
    // native info
    void*            data;
    skr::rttr::Type* type;
    EV8RecordOwner   owner;

    // v8 info
    ::v8::Persistent<::v8::Object> v8_object;
};
} // namespace skr::v8