#pragma once
#include "SkrRTTR/script_binder.hpp"
#include "v8-persistent-handle.h"
#include "v8-template.h"
#include "SkrRTTR/type.hpp"
#include "SkrRTTR/scriptble_object.hpp"

namespace skr
{
//===============================bind data===============================
struct V8BindMethodData {
    ScriptBinderMethod binder;
};
struct V8BindStaticMethodData {
    ScriptBinderStaticMethod binder;
};
struct V8BindFieldData {
    ScriptBinderField binder;
};
struct V8BindStaticFieldData {
    ScriptBinderStaticField binder;
};
struct V8BindPropertyData {
    ScriptBinderProperty binder;
};
struct V8BindStaticPropertyData {
    ScriptBinderStaticProperty binder;
};
struct V8BindRecordDataBase {
    // v8 info
    ::v8::Global<::v8::FunctionTemplate> ctor_template;

    // native info
    ScriptBinderObject*                    binder;
    Map<String, V8BindMethodData*>         methods;
    Map<String, V8BindFieldData*>          fields;
    Map<String, V8BindStaticMethodData*>   static_methods;
    Map<String, V8BindStaticFieldData*>    static_fields;
    Map<String, V8BindPropertyData*>       properties;
    Map<String, V8BindStaticPropertyData*> static_properties;

    ~V8BindRecordDataBase()
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
        for (auto& pair : properties)
        {
            SkrDelete(pair.value);
        }
        for (auto& pair : static_properties)
        {
            SkrDelete(pair.value);
        }
    }
};
struct V8BindObjectData : V8BindRecordDataBase {
};
struct V8BindValueData : V8BindRecordDataBase {
};
struct V8BindEnumData {
    // v8 info
    v8::Global<v8::ObjectTemplate> enum_template;

    ScriptBinderEnum* binder;
};

//===============================bind core===============================
// TODO. V8BindRecordCoreBase, 用于复用逻辑
struct V8BindObjectCore {
    // native info
    skr::ScriptbleObject* object      = nullptr;
    void*                 object_head = nullptr;
    const skr::RTTRType*  type        = nullptr;

    // v8 info
    ::v8::Persistent<::v8::Object> v8_object;

    // helper functions
    inline void* cast_to_base(::skr::GUID type_id)
    {
        return type->cast_to_base(type_id, object->iobject_get_head_ptr());
    }
};
struct V8BindValueCore {
};
} // namespace skr