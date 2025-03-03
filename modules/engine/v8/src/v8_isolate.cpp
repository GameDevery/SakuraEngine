#include "SkrV8/v8_isolate.hpp"
#include "SkrContainers/set.hpp"
#include "SkrV8/v8_bind_tools.hpp"
#include "SkrV8/v8_bind_data.hpp"
#include "libplatform/libplatform.h"
#include "v8-initialization.h"
#include "SkrRTTR/type.hpp"
#include "v8-template.h"
#include "v8-external.h"
#include "v8-function.h"

// allocator
namespace skr::v8
{
struct V8Allocator final : ::v8::ArrayBuffer::Allocator {
    static constexpr const char* kV8DefaultPoolName = "v8-allocate";

    void* AllocateUninitialized(size_t length) override
    {
#if defined(TRACY_TRACE_ALLOCATION)
        SkrCZoneNCS(z, "v8::allocate", SKR_ALLOC_TRACY_MARKER_COLOR, 16, 1);
        void* p = sakura_malloc_alignedN(length, alignof(size_t), kV8DefaultPoolName);
        SkrCZoneEnd(z);
        return p;
#else
        return sakura_malloc_aligned(length, alignof(size_t));
#endif
    }
    void Free(void* data, size_t length) override
    {
        if (data)
        {
#if defined(TRACY_TRACE_ALLOCATION)
            SkrCZoneNCS(z, "v8::free", SKR_DEALLOC_TRACY_MARKER_COLOR, 16, 1);
            sakura_free_alignedN(data, alignof(size_t), kV8DefaultPoolName);
            SkrCZoneEnd(z);
#else
            sakura_free_aligned(data, alignof(size_t));
#endif
        }
    }
    void* Reallocate(void* data, size_t old_length, size_t new_length) override
    {
        SkrCZoneNCS(z, "v8::realloc", SKR_DEALLOC_TRACY_MARKER_COLOR, 16, 1);
        void* new_mem = sakura_realloc_alignedN(data, new_length, alignof(size_t), kV8DefaultPoolName);
        SkrCZoneEnd(z);
        return new_mem;
    }
    void* Allocate(size_t length) override
    {
        void* p = AllocateUninitialized(length);
        memset(p, 0, length);
        return p;
    }
};
} // namespace skr::v8

namespace skr::v8
{
V8Isolate::V8Isolate()
{
}
V8Isolate::~V8Isolate()
{
}

void V8Isolate::init()
{
    using namespace ::v8;

    // init isolate
    _isolate_create_params.array_buffer_allocator = SkrNew<V8Allocator>();
    _isolate                                      = Isolate::New(_isolate_create_params);
    _isolate->SetData(0, this);
}
void V8Isolate::shutdown()
{
    if (_isolate)
    {
        // clear templates
        _record_templates.clear();

        // dispose isolate
        _isolate->Dispose();

        // delete array buffer allocator
        SkrDelete(_isolate_create_params.array_buffer_allocator);
    }
}

// TODO. 遍历父类结构的绑定，以及重载实现
void V8Isolate::make_record_template(::skr::rttr::Type* type)
{
    using namespace ::v8;
    SKR_ASSERT(type->type_category() == ::skr::rttr::ETypeCategory::Record);
    Isolate::Scope isolate_scope(_isolate);
    HandleScope    handle_scope(_isolate);

    // ctor template
    auto ctor_template = FunctionTemplate::New(
        _isolate,
        _call_ctor,
        External::New(_isolate, type)
    );

    // setup internal field count
    ctor_template->InstanceTemplate()->SetInternalFieldCount(1);
    
    // bind member component
    {
        // bind method
        type->each_method([&](const rttr::MethodData* method){
            ctor_template->PrototypeTemplate()->Set(
                ::v8::String::NewFromUtf8(_isolate, method->name.c_str_raw()).ToLocalChecked(),
                FunctionTemplate::New(
                    _isolate,
                    _call_method,
                    External::New(_isolate, const_cast<rttr::MethodData*>(method))
                )
            );
        });

        // bind field
        type->each_field([&](const rttr::FieldData* field){
            ctor_template->PrototypeTemplate()->SetAccessorProperty(
                ::v8::String::NewFromUtf8(_isolate, field->name.c_str_raw()).ToLocalChecked(),
                FunctionTemplate::New(
                    _isolate,
                    _get_field,
                    External::New(_isolate, const_cast<rttr::FieldData*>(field))
                ),
                FunctionTemplate::New(
                    _isolate,
                    _set_field,
                    External::New(_isolate, const_cast<rttr::FieldData*>(field))
                )
            );
        });
    }

    // bind static component
    {
        // bind static method
        type->each_static_method([&](const rttr::StaticMethodData* static_method){
            ctor_template->Set(
                ::v8::String::NewFromUtf8(_isolate, static_method->name.c_str_raw()).ToLocalChecked(),
                FunctionTemplate::New(
                    _isolate,
                    _call_static_method,
                    External::New(_isolate, const_cast<rttr::StaticMethodData*>(static_method))
                )
            );
        });

        // bind static field
        type->each_static_field([&](const rttr::StaticFieldData* static_field){
            ctor_template->SetAccessorProperty(
                ::v8::String::NewFromUtf8(_isolate, static_field->name.c_str_raw()).ToLocalChecked(),
                FunctionTemplate::New(
                    _isolate,
                    _get_static_field,
                    External::New(_isolate, const_cast<rttr::StaticFieldData*>(static_field))
                ),
                FunctionTemplate::New(
                    _isolate,
                    _set_static_field,
                    External::New(_isolate, const_cast<rttr::StaticFieldData*>(static_field))
                )
            );
        });
    }

    // store template
    auto& template_ref = _record_templates.try_add_default(type).value();
    template_ref.Reset(_isolate, ctor_template);
}
void V8Isolate::inject_templates_into_context(::v8::Local<::v8::Context> context)
{
    ::v8::Isolate::Scope isolate_scope(_isolate);
    ::v8::HandleScope    handle_scope(_isolate);

    for (const auto& pair : _record_templates)
    {
        const auto& type         = pair.key;
        const auto& template_ref = pair.value;

        // make function template
        auto function = template_ref.Get(_isolate)->GetFunction(context).ToLocalChecked();

        // set to context
        context->Global()->Set(
                             context,
                             ::v8::String::NewFromUtf8(_isolate, type->name().c_str_raw()).ToLocalChecked(),
                             function)
            .Check();
    }
}

// bind object
V8BindRecordCore* V8Isolate::translate_record(::skr::rttr::ScriptbleObject* obj)
{
    using namespace ::v8;
    Isolate::Scope isolate_scope(_isolate);
    HandleScope    handle_scope(_isolate);
    Local<Context> context = _isolate->GetCurrentContext();

    // find exist object
    auto bind_ref = _alive_records.find(obj);
    if (bind_ref)
    {
        return bind_ref.value();
    }

    // get type
    auto type = rttr::get_type_from_guid(obj->iobject_get_typeid());

    // get template
    auto template_ref = _record_templates.find(type);
    if (!template_ref)
    {
        return nullptr;
    }

    // make object
    Local<Function> ctor_func = template_ref.value().Get(_isolate)->GetFunction(context).ToLocalChecked();
    Local<Object> object = ctor_func->NewInstance(context).ToLocalChecked();

    // make bind data
    auto bind_data = SkrNew<V8BindRecordCore>();
    bind_data->object = obj;
    bind_data->type   = type;
    bind_data->v8_object.Reset(_isolate, object);

    // setup gc callback
    bind_data->v8_object.SetWeak(
        bind_data,
        _gc_callback,
        WeakCallbackType::kInternalFields
    );

    // add extern data
    object->SetInternalField(0, External::New(_isolate, bind_data));

    // add to map
    _alive_records.add(obj, bind_data);

    return bind_data;
}
void V8Isolate::mark_record_deleted(::skr::rttr::ScriptbleObject* obj)
{
    auto bind_ref = _alive_records.find(obj);
    if (bind_ref)
    {
        // reset core object ptr
        bind_ref.value()->object = nullptr;

        // move to deleted records
        _deleted_records.push_back(bind_ref.value());
        _alive_records.remove(obj);
    }
}

// bind helpers
void V8Isolate::_gc_callback(const ::v8::WeakCallbackInfo<V8BindRecordCore>& data)
{
    using namespace ::v8;
    
    // get data
    V8BindRecordCore* bind_core = data.GetParameter();
    V8Isolate* isolate = reinterpret_cast<V8Isolate*>(data.GetIsolate()->GetData(0));
    
    // delete if has owner ship
    if (bind_core->object->script_owner_ship() == rttr::EScriptbleObjectOwnerShip::Script)
    {
        SkrDelete(bind_core->object);
    }

    // remove from isolate
    isolate->_alive_records.remove(bind_core->object);
    
    // destroy it
    SkrDelete(bind_core);
}
void V8Isolate::_call_ctor(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    // get v8 basic info
    Isolate*       Isolate = info.GetIsolate();
    Isolate::Scope IsolateScope(Isolate);
    HandleScope    HandleScope(Isolate);
    Local<Context> Context = Isolate->GetCurrentContext();
    Context::Scope ContextScope(Context);

    if (info.IsConstructCall())
    {
        // get ctor info
        Local<Object> self = info.This();

        // get type info
        Local<External>  data = info.Data().As<External>();
        skr::rttr::Type* type = reinterpret_cast<skr::rttr::Type*>(data->Value());

        // match ctor
        bool done_ctor = false;
        type->each_ctor([&](const rttr::CtorData* ctor_data){
            if (done_ctor) return;
            
            if (V8BindTools::match_params(ctor_data, info))
            {
                V8Isolate* skr_isolate = reinterpret_cast<V8Isolate*>(Isolate->GetData(0));

                // alloc memory
                void* alloc_mem = sakura_new_aligned(type->size(), type->alignment());

                // call ctor
                V8BindTools::call_ctor(alloc_mem, *ctor_data, info, Context, Isolate);
                
                // cast to ScriptbleObject
                void* casted_mem = type->cast_to(rttr::type_id_of<rttr::ScriptbleObject>(), alloc_mem);

                // make bind core
                V8BindRecordCore* bind_core = SkrNew<V8BindRecordCore>();
                bind_core->type = type;
                bind_core->object = reinterpret_cast<rttr::ScriptbleObject*>(casted_mem);

                // setup owner ship
                bind_core->object->script_owner_ship_take(rttr::EScriptbleObjectOwnerShip::Script);

                // setup gc callback
                bind_core->v8_object.Reset(Isolate, self);
                bind_core->v8_object.SetWeak(
                    bind_core,
                    _gc_callback,
                    WeakCallbackType::kInternalFields
                );

                // add extern data
                self->SetInternalField(0, External::New(Isolate, bind_core));

                // add to map
                skr_isolate->_alive_records.add(bind_core->object, bind_core);

                done_ctor = true;
            }
        });

        // no ctor matched
        Isolate->ThrowError("no ctor matched");
    }
    else
    {
        Isolate->ThrowError("must be called with new");
    }
}
void V8Isolate::_call_method(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
}
void V8Isolate::_get_field(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    // using namespace ::v8;

    // // get data
    // Isolate*          isolate = info.GetIsolate();
    // Local<Context>    context = isolate->GetCurrentContext();
    // Local<Object>     self    = info.This();
    // Local<External>   wrap    = Local<External>::Cast(self->GetInternalField(0));
    // V8BindRecordCore* bind_core    = reinterpret_cast<V8BindRecordCore*>(wrap->Value());

    // // get field data
    // auto   name_len = property->Utf8Length(isolate);
    // String field_name;
    // field_name.resize_unsafe(name_len);
    // property->WriteUtf8(isolate, field_name.data_raw_w());
    // auto& field_data = bind_core->type->record_data().fields.find_if(
    // [&](const auto& f) {
    //         return f->name == field_name;
    //     }).ref();

    // // return field
    // Local<Value> result;
    // if (V8BindTools::native_to_v8_primitive(
    //         context,
    //         isolate,
    //         field_data->type,
    //         field_data->get_address(bind_core->object),
    //         result))
    // {
    //     info.GetReturnValue().Set(result);
    // }
    // else
    // {
    //     isolate->ThrowError("field type not supported");
    // }
}
void V8Isolate::_set_field(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    // using namespace ::v8;

    // // get data
    // Isolate*          isolate = info.GetIsolate();
    // Local<Context>    context = isolate->GetCurrentContext();
    // Local<Object>     self    = info.This();
    // Local<External>   wrap    = Local<External>::Cast(self->GetInternalField(0));
    // V8BindRecordCore* bind_core    = reinterpret_cast<V8BindRecordCore*>(wrap->Value());

    // // get field data
    // auto   name_len = property->Utf8Length(isolate);
    // String field_name;
    // field_name.add('\0', name_len);
    // property->WriteUtf8(isolate, field_name.data_raw_w());
    // auto& field_data = bind_core->type->record_data().fields.find_if([&](const auto& f) {
    //                                                        return f->name == field_name;
    //                                                    })
    //                        .ref();

    // // set field
    // if (V8BindTools::v8_to_native_primitive(
    //         context,
    //         isolate,
    //         field_data->type,
    //         value,
    //         field_data->get_address(bind_core->object)))
    // {
    //     info.GetReturnValue().Set(value);
    // }
    // else
    // {
    //     isolate->ThrowError("field type not supported");
    // }
}
void V8Isolate::_call_static_method(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
}
void V8Isolate::_get_static_field(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
}
void V8Isolate::_set_static_field(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
}
} // namespace skr::v8

namespace skr::v8
{
static auto& _v8_platform()
{
    static auto _platform = ::v8::platform::NewDefaultPlatform();
    return _platform;
}

void init_v8()
{
    // init flags
    // char Flags[] = "--expose-gc";
    // ::v8::V8::SetFlagsFromString(Flags, sizeof(Flags));

    // init platform
    _v8_platform() = ::v8::platform::NewDefaultPlatform();
    ::v8::V8::InitializePlatform(_v8_platform().get());

    // init v8
    ::v8::V8::Initialize();
}
void shutdown_v8()
{
    // shutdown v8
    ::v8::V8::Dispose();

    // shutdown platform
    ::v8::V8::DisposePlatform();
    _v8_platform().reset();
}
} // namespace skr::v8