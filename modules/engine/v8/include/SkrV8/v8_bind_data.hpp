#pragma once
#include "SkrRTTR/script_binder.hpp"
#include "v8-persistent-handle.h"
#include "v8-template.h"
#include "SkrRTTR/type.hpp"
#include "SkrRTTR/scriptble_object.hpp"

namespace skr
{
//===============================bind data===============================
struct V8BindDataMethod {
    ScriptBinderMethod binder;
};
struct V8BindDataStaticMethod {
    ScriptBinderStaticMethod binder;
};
struct V8BindDataField {
    ScriptBinderField binder;
};
struct V8BindDataStaticField {
    ScriptBinderStaticField binder;
};
struct V8BindDataProperty {
    ScriptBinderProperty binder;
};
struct V8BindDataStaticProperty {
    ScriptBinderStaticProperty binder;
};
struct V8BindDataObject;
struct V8BindDataValue;
struct V8BindDataRecordBase {
    // v8 info
    ::v8::Global<::v8::FunctionTemplate> ctor_template;

    // native info
    bool                                   is_value = false;
    Map<String, V8BindDataMethod*>         methods;
    Map<String, V8BindDataField*>          fields;
    Map<String, V8BindDataStaticMethod*>   static_methods;
    Map<String, V8BindDataStaticField*>    static_fields;
    Map<String, V8BindDataProperty*>       properties;
    Map<String, V8BindDataStaticProperty*> static_properties;

    V8BindDataObject* as_object();
    V8BindDataValue*  as_value();

    inline ~V8BindDataRecordBase()
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
struct V8BindDataObject : V8BindDataRecordBase {
    inline V8BindDataObject()
    {
        is_value = false;
    }

    ScriptBinderObject* binder;
};
struct V8BindDataValue : V8BindDataRecordBase {
    inline V8BindDataValue()
    {
        is_value = true;
    }

    ScriptBinderValue* binder;
};
struct V8BindDataEnum {
    // v8 info
    v8::Global<v8::ObjectTemplate> enum_template;

    ScriptBinderEnum* binder;
};
inline V8BindDataObject* V8BindDataRecordBase::as_object()
{
    SKR_ASSERT(!is_value);
    return static_cast<V8BindDataObject*>(this);
}
inline V8BindDataValue* V8BindDataRecordBase::as_value()
{
    SKR_ASSERT(is_value);
    return static_cast<V8BindDataValue*>(this);
}

//===============================bind core===============================
struct V8BindCoreObject;
struct V8BindCoreValue;
struct V8BindCoreRecordBase {
    // type
    V8BindCoreObject* as_object();
    V8BindCoreValue*  as_value();

    // type
    bool is_value = false;

    // record data
    const skr::RTTRType* type = nullptr;
    void*                data = nullptr;

    // fields
    Map<void*, V8BindCoreValue*> cache_value_fields = {};

    // v8 info
    ::v8::Persistent<::v8::Object> v8_object = {};

    ~V8BindCoreRecordBase();
};
struct V8BindCoreObject : V8BindCoreRecordBase {
    inline V8BindCoreObject()
    {
        is_value = false;
    }

    // native info
    skr::ScriptbleObject* object = nullptr;

    // helper functions
    inline void* cast_to_base(::skr::GUID type_id)
    {
        return type->cast_to_base(type_id, data);
    }
};
struct V8BindCoreValue : V8BindCoreRecordBase {
    inline V8BindCoreValue()
    {
        is_value = true;
    }

    // owner info
    V8BindCoreRecordBase* owner_core          = nullptr;
    bool                  owner_core_released = false;
    bool                  from_static_field   = false; // TODO. 暂时未集成
    bool                  from_param          = false; // TODO. 暂时未集成
    bool                  is_param_invalid    = false; // TODO. 暂时未集成
    // TODO. 改为如下结构
    // ESource
    //   Field
    //   StaticField
    //   Param
    //   V8New
    // bool is_source_valid
    // V8BindCoreRecordBase* field_owner; // used to delete cache

    inline bool has_owner()
    {
        return owner_core != nullptr;
    }
};

inline V8BindCoreObject* V8BindCoreRecordBase::as_object()
{
    SKR_ASSERT(!is_value);
    return static_cast<V8BindCoreObject*>(this);
}
inline V8BindCoreValue* V8BindCoreRecordBase::as_value()
{
    SKR_ASSERT(is_value);
    return static_cast<V8BindCoreValue*>(this);
}
inline V8BindCoreRecordBase::~V8BindCoreRecordBase()
{
    for (auto& [field_ptr, field_core] : cache_value_fields)
    {
        field_core->owner_core_released = true;
    }
}
} // namespace skr