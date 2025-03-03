#pragma once
#include "SkrRTTR/type.hpp"
#include "v8-persistent-handle.h"
#include "SkrRTTR/scriptble_object.hpp"

namespace skr::v8
{
struct V8BindRecordCore;

struct V8BindMethodCore {
    struct OverloadInfo {
        skr::rttr::Type* type;
        skr::rttr::MethodData* method;
    };

    // native info
    String name;
    Vector<OverloadInfo> overloads;

    // owner record bind core
    V8BindRecordCore* owner;
};

struct V8BindStaticMethodCore {
    struct OverloadInfo {
        skr::rttr::Type* type;
        skr::rttr::StaticMethodData* method;
    };

    // native info
    String name;
    Vector<OverloadInfo> overloads;

    // owner record bind core
    V8BindRecordCore* owner;
};

struct V8BindFieldCore {
    // native info
    String name;
    skr::rttr::Type* type;
    skr::rttr::FieldData* field;

    // owner record bind core
    V8BindRecordCore* owner;
};

struct V8BindStaticFieldCore {
    // native info
    String name;
    skr::rttr::Type* type;
    skr::rttr::StaticFieldData* field;

    // owner record bind core
    V8BindRecordCore* owner;
};

struct V8BindRecordCore {
    // native info
    skr::rttr::ScriptbleObject* object;
    skr::rttr::Type*            type;

    // v8 info
    ::v8::Persistent<::v8::Object> v8_object;

    // field & method
    Map<String, V8BindMethodCore*> methods;
    Map<String, V8BindFieldCore*>  fields;

    // static field & static method
    Map<String, V8BindStaticMethodCore*> static_methods;
    Map<String, V8BindStaticFieldCore*>  static_fields;
};
} // namespace skr::v8