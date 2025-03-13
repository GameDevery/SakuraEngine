#pragma once
#include "v8-persistent-handle.h"
#include "v8-template.h"

#include "SkrRTTR/type.hpp"
#include "SkrRTTR/scriptble_object.hpp"

namespace skr
{
//===============================bind data===============================
struct V8BindMethodData {
    struct OverloadInfo {
        const skr::rttr::Type*       owner_type;
        const skr::rttr::MethodData* method;
    };

    // native info
    String               name;
    Vector<OverloadInfo> overloads;
};
struct V8BindStaticMethodData {
    struct OverloadInfo {
        const skr::rttr::Type*             owner_type;
        const skr::rttr::StaticMethodData* method;
    };

    // native info
    String               name;
    Vector<OverloadInfo> overloads;
};
struct V8BindFieldData {
    // native info
    String                      name;
    const skr::rttr::Type*      owner_type;
    const skr::rttr::FieldData* field;
};
struct V8BindStaticFieldData {
    // native info
    String                            name;
    const skr::rttr::Type*            owner_type;
    const skr::rttr::StaticFieldData* field;
};
struct V8BindRecordData {
    // v8 info
    ::v8::Global<::v8::FunctionTemplate> ctor_template;

    // native info
    skr::rttr::Type*                     type;
    Map<String, V8BindMethodData*>       methods;
    Map<String, V8BindFieldData*>        fields;
    Map<String, V8BindStaticMethodData*> static_methods;
    Map<String, V8BindStaticFieldData*>  static_fields;

    ~V8BindRecordData()
    {
        for (auto& pair : methods)
        {
            SkrDelete(pair.value);
        }
        for (auto& pair : fields)
        {
            SkrDelete(pair.value);
        }
        for (auto& pair : static_methods)
        {
            SkrDelete(pair.value);
        }
        for (auto& pair : static_fields)
        {
            SkrDelete(pair.value);
        }
    }
};

//===============================bind core===============================
struct V8BindRecordCore {
    // native info
    skr::rttr::ScriptbleObject* object;
    skr::rttr::Type*            type;

    // v8 info
    ::v8::Persistent<::v8::Object> v8_object;
    
    // helper functions
    inline void* cast_to_base(::skr::GUID type_id)
    {
        return type->cast_to_base(type_id, object->iobject_get_head_ptr());
    }
};
} // namespace skr