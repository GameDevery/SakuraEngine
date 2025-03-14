#include "SkrV8/v8_isolate.hpp"
#include "SkrContainers/set.hpp"
#include "SkrCore/log.hpp"
#include "SkrV8/v8_bind_tools.hpp"
#include "SkrV8/v8_bind_data.hpp"
#include "libplatform/libplatform.h"
#include "v8-initialization.h"
#include "SkrRTTR/type.hpp"
#include "v8-template.h"
#include "v8-external.h"
#include "v8-function.h"

// allocator
namespace skr
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
} // namespace skr

namespace skr
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

        // clean up core
        for (auto& [obj, bind_core] : _alive_records)
        {
            if (bind_core->object->script_owner_ship() == rttr::EScriptbleObjectOwnerShip::Script)
            {
                SkrDelete(bind_core->object);
            }

            SkrDelete(bind_core);
        }
        _alive_records.clear();
        for (auto& bind_core : _deleted_records)
        {
            SkrDelete(bind_core);
        }

        // clean up templates
        for (auto& [type, bind_data] : _record_templates)
        {
            SkrDelete(bind_data);
        }
    }
}

// operator isolate
void V8Isolate::gc(bool full)
{
    _isolate->RequestGarbageCollectionForTesting(full ? ::v8::Isolate::kFullGarbageCollection : ::v8::Isolate::kMinorGarbageCollection);
    
    _isolate->LowMemoryNotification();
    _isolate->IdleNotificationDeadline(0);
}

// register type
void V8Isolate::make_record_template(::skr::rttr::Type* type)
{
    using namespace ::v8;

    // check
    SKR_ASSERT(type->type_category() == ::skr::rttr::ETypeCategory::Record);
    SKR_ASSERT(type->based_on(type_id_of<rttr::ScriptbleObject>()));

    // find exist template
    if (_record_templates.contains(type))
    {
        return;
    }

    // v8 scope
    Isolate::Scope isolate_scope(_isolate);
    HandleScope    handle_scope(_isolate);

    // new bind data
    auto bind_data = SkrNew<V8BindRecordData>();
    bind_data->type = type;

    // ctor template
    auto ctor_template = FunctionTemplate::New(
        _isolate,
        _call_ctor,
        External::New(_isolate, bind_data)
    );
    bind_data->ctor_template.Reset(_isolate, ctor_template);

    // setup internal field count
    ctor_template->InstanceTemplate()->SetInternalFieldCount(1);

    // bind method
    type->each_method([&](const rttr::MethodData* method, const rttr::Type* owner_type){
        if (skr::flag_all(method->flag, rttr::EMethodFlag::ScriptVisible))
        {
            // find exist method bind data
            V8BindMethodData* data = nullptr;
            {
                if (auto ref = bind_data->methods.find(method->name))
                {
                    data = ref.value();
                }
                else
                {
                    data = SkrNew<V8BindMethodData>();
                    data->name = method->name;
                    bind_data->methods.add(method->name, data);
                    
                    // bind to template
                    ctor_template->PrototypeTemplate()->Set(
                        V8BindTools::str_to_v8(method->name, _isolate, true),
                        FunctionTemplate::New(
                            _isolate,
                            _call_method,
                            External::New(_isolate, data)
                        )
                    );
                }
            }

            // add overload
            data->overloads.add({
                .owner_type  = owner_type,
                .method = method
            });
        }
    });

    // bind field
    type->each_field([&](const rttr::FieldData* field, const rttr::Type* owner_type){
        if (skr::flag_all(field->flag, rttr::EFieldFlag::ScriptVisible))
        {
            // find exist field bind data
            V8BindFieldData* data = nullptr;
            {
                if (auto ref = bind_data->fields.find(field->name))
                {
                    SKR_LOG_FMT_ERROR(u8"field named '{}', already exist in type '{}'", field->name, owner_type->name());
                    data = ref.value();
                }
                else
                {
                    data = SkrNew<V8BindFieldData>();
                    data->name = field->name;
                    bind_data->fields.add(field->name, data);

                    // bind to template
                    ctor_template->PrototypeTemplate()->SetAccessorProperty(
                        V8BindTools::str_to_v8(field->name, _isolate, true),
                        FunctionTemplate::New(
                            _isolate,
                            _get_field,
                            External::New(_isolate, data)
                        ),
                        FunctionTemplate::New(
                            _isolate,
                            _set_field,
                            External::New(_isolate, data)
                        )
                    );
                }
            }

            // fill data
            data->owner_type  = owner_type;
            data->field = field;
        }
    });

    // bind static method
    type->each_static_method([&](const rttr::StaticMethodData* static_method, const rttr::Type* owner_type){
        if (skr::flag_all(static_method->flag, rttr::EStaticMethodFlag::ScriptVisible))
        {
            // find exist static method bind data
            V8BindStaticMethodData* data = nullptr;
            {
                if (auto ref = bind_data->static_methods.find(static_method->name))
                {
                    data = ref.value();
                }
                else
                {
                    data = SkrNew<V8BindStaticMethodData>();
                    data->name = static_method->name;
                    bind_data->static_methods.add(static_method->name, data);

                    // bind to template
                    ctor_template->Set(
                        V8BindTools::str_to_v8(static_method->name, _isolate, true),
                        FunctionTemplate::New(
                            _isolate,
                            _call_static_method,
                            External::New(_isolate, data)
                        )
                    );
                }
            }

            // add overload
            data->overloads.add({
                .owner_type = owner_type,
                .method = static_method
            });
        }
    });

    // bind static field
    type->each_static_field([&](const rttr::StaticFieldData* static_field, const rttr::Type* owner_type){
        if (skr::flag_all(static_field->flag, rttr::EStaticFieldFlag::ScriptVisible))
        {
            // find exist static field bind data
            V8BindStaticFieldData* data = nullptr;
            {
                if (auto ref = bind_data->static_fields.find(static_field->name))
                {
                    data = ref.value();
                }
                else
                {
                    data = SkrNew<V8BindStaticFieldData>();
                    data->name = static_field->name;
                    bind_data->static_fields.add(static_field->name, data);
                    
                    // bind to template
                    ctor_template->SetAccessorProperty(
                        V8BindTools::str_to_v8(static_field->name, _isolate, true),
                        FunctionTemplate::New(
                            _isolate,
                            _get_static_field,
                            External::New(_isolate, data)
                        ),
                        FunctionTemplate::New(
                            _isolate,
                            _set_static_field,
                            External::New(_isolate, data)
                        )
                    );
                }
            }

            // fill data
            data->owner_type  = owner_type;
            data->field = static_field;
        }
    });

    // store template
    _record_templates.add(type, bind_data);
}
void V8Isolate::inject_templates_into_context(::v8::Global<::v8::Context> context)
{
    ::v8::Isolate::Scope       isolate_scope(_isolate);
    ::v8::HandleScope          handle_scope(_isolate);
    ::v8::Local<::v8::Context> local_context = context.Get(_isolate);

    for (const auto& pair : _record_templates)
    {
        const auto& type         = pair.key;
        const auto& template_ref = pair.value->ctor_template;

        // make function template
        auto function = template_ref.Get(_isolate)->GetFunction(local_context).ToLocalChecked();

        // set to context
        local_context->Global()->Set(
            local_context,
            V8BindTools::str_to_v8(type->name(), _isolate, true),
            function
        ).Check();
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
    Local<Function> ctor_func = template_ref.value()->ctor_template.Get(_isolate)->GetFunction(context).ToLocalChecked();
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
    
    // remove alive object
    if (bind_core->object)
    {
        // delete if has owner ship
        if (bind_core->object->script_owner_ship() == rttr::EScriptbleObjectOwnerShip::Script)
        {
            SkrDelete(bind_core->object);
        }

        isolate->_alive_records.remove(bind_core->object);
    }
    else
    {
        // remove from deleted records
        isolate->_deleted_records.remove(bind_core);
    }

    // reset object handle
    bind_core->v8_object.Reset();

    // destroy it
    SkrDelete(bind_core);
}
void V8Isolate::_call_ctor(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    // get v8 basic info
    Isolate*       Isolate = info.GetIsolate();
    Local<Context> Context = Isolate->GetCurrentContext();

    // scopes
    Isolate::Scope IsolateScope(Isolate);
    HandleScope    HandleScope(Isolate);
    Context::Scope ContextScope(Context);

    // get user data
    auto* bind_data   = reinterpret_cast<V8BindRecordData*>(info.Data().As<External>()->Value());
    auto* skr_isolate = reinterpret_cast<V8Isolate*>(Isolate->GetData(0));

    // handle call
    if (info.IsConstructCall())
    {
        // get ctor info
        Local<Object> self = info.This();

        // check constructable
        if (!flag_any(bind_data->type->record_flag(), rttr::ERecordFlag::ScriptNewable))
        {
            Isolate->ThrowError("record is not constructable");
            return;
        }

        // match ctor
        bool done_ctor = false;
        bind_data->type->each_ctor([&](const rttr::CtorData* ctor_data){
            if (done_ctor) return;
            
            if (V8BindTools::match_params(ctor_data, info))
            {
                // alloc memory
                void* alloc_mem = sakura_new_aligned(bind_data->type->size(), bind_data->type->alignment());

                // call ctor
                V8BindTools::call_ctor(alloc_mem, *ctor_data, info, Context, Isolate);
                
                // cast to ScriptbleObject
                void* casted_mem = bind_data->type->cast_to_base(type_id_of<rttr::ScriptbleObject>(), alloc_mem);

                // make bind core
                V8BindRecordCore* bind_core = SkrNew<V8BindRecordCore>();
                bind_core->type = bind_data->type;
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

        if (!done_ctor)
        {
            Isolate->ThrowError("no ctor matched");
        }
    }
    else
    {
        Isolate->ThrowError("must be called with new");
    }
}
void V8Isolate::_call_method(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    // get v8 basic info
    Isolate*       Isolate = info.GetIsolate();
    Local<Context> Context = Isolate->GetCurrentContext();

    // scopes
    Isolate::Scope IsolateScope(Isolate);
    HandleScope    HandleScope(Isolate);
    Context::Scope ContextScope(Context);

    // get self data
    Local<Object> self      = info.This();
    auto*         bind_core = reinterpret_cast<V8BindRecordCore*>(self->GetInternalField(0).As<External>()->Value());

    // get user data
    auto* bind_data   = reinterpret_cast<V8BindMethodData*>(info.Data().As<External>()->Value());
    auto* skr_isolate = reinterpret_cast<V8Isolate*>(Isolate->GetData(0));

    // block ctor call
    if (info.IsConstructCall())
    {
        Isolate->ThrowError("method can not be called with new");
        return;
    }

    // match method
    for (const auto& overload: bind_data->overloads)
    {
        if (V8BindTools::match_params(overload.method, info))
        {
            void* obj =  bind_core->cast_to_base(overload.owner_type->type_id());

            V8BindTools::call_method(
                obj,
                overload.method->param_data,
                overload.method->ret_type,
                overload.method->dynamic_stack_invoke,
                info,
                Context,
                Isolate
            );

            break;
        }
    }
}
void V8Isolate::_call_static_method(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    // get v8 basic info
    Isolate*       Isolate = info.GetIsolate();
    Local<Context> Context = Isolate->GetCurrentContext();

    // scopes
    Isolate::Scope IsolateScope(Isolate);
    HandleScope    HandleScope(Isolate);
    Context::Scope ContextScope(Context);

    // get user data
    auto* bind_data   = reinterpret_cast<V8BindStaticMethodData*>(info.Data().As<External>()->Value());
    auto* skr_isolate = reinterpret_cast<V8Isolate*>(Isolate->GetData(0));

    // block ctor call
    if (info.IsConstructCall())
    {
        Isolate->ThrowError("method can not be called with new");
        return;
    }

    // match method
    for (const auto& overload: bind_data->overloads)
    {
        if (V8BindTools::match_params(overload.method, info))
        {
            V8BindTools::call_function(
                overload.method->param_data,
                overload.method->ret_type,
                overload.method->dynamic_stack_invoke,
                info,
                Context,
                Isolate
            );

            break;
        }
    }
}
void V8Isolate::_get_field(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    // get data
    Isolate*       Isolate = info.GetIsolate();
    Local<Context> Context = Isolate->GetCurrentContext();

    // scopes
    Isolate::Scope IsolateScope(Isolate);
    HandleScope    HandleScope(Isolate);
    Context::Scope ContextScope(Context);

    // get self data
    Local<Object> self      = info.This();
    auto*         bind_core = reinterpret_cast<V8BindRecordCore*>(self->GetInternalField(0).As<External>()->Value());

    // get user data
    auto* bind_data   = reinterpret_cast<V8BindFieldData*>(info.Data().As<External>()->Value());
    auto* skr_isolate = reinterpret_cast<V8Isolate*>(Isolate->GetData(0));

    // get field address
    void* field_address = bind_data->field->get_address(bind_core->cast_to_base(bind_data->owner_type->type_id()));

    // return field
    Local<Value> result;
    if (V8BindTools::native_to_v8_primitive(
        Context,
        Isolate,
        bind_data->field->type,
        field_address,
        result
    ))
    {
        info.GetReturnValue().Set(result);
    }
    else if (V8BindTools::native_to_v8_box(
        Context,
        Isolate,
        bind_data->field->type,
        field_address,
        result
    ))
    {
        info.GetReturnValue().Set(result);
    }
    else
    {
        Isolate->ThrowError("field type not supported");
    }
}
void V8Isolate::_set_field(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    // get data
    Isolate*       Isolate = info.GetIsolate();
    Local<Context> Context = Isolate->GetCurrentContext();

    // scopes
    Isolate::Scope IsolateScope(Isolate);
    HandleScope    HandleScope(Isolate);
    Context::Scope ContextScope(Context);

    // get self data
    Local<Object> self      = info.This();
    auto*         bind_core = reinterpret_cast<V8BindRecordCore*>(self->GetInternalField(0).As<External>()->Value());

    // get user data
    auto* bind_data   = reinterpret_cast<V8BindFieldData*>(info.Data().As<External>()->Value());
    auto* skr_isolate = reinterpret_cast<V8Isolate*>(Isolate->GetData(0));

    // get field address
    void* field_address = bind_data->field->get_address(bind_core->cast_to_base(bind_data->owner_type->type_id()));

    // set field
    if (V8BindTools::v8_to_native_primitive(
        Context,
        Isolate,
        bind_data->field->type,
        info[0],
        field_address,
        true
    ))
    {
    }
    else if (V8BindTools::v8_to_native_box(
        Context,
        Isolate,
        bind_data->field->type,
        info[0],
        field_address,
        true
    ))
    {
    }
    else
    {
        Isolate->ThrowError("field type not supported");
    }
}
void V8Isolate::_get_static_field(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    // get data
    Isolate*       Isolate = info.GetIsolate();
    Local<Context> Context = Isolate->GetCurrentContext();

    // scopes
    Isolate::Scope IsolateScope(Isolate);
    HandleScope    HandleScope(Isolate);
    Context::Scope ContextScope(Context);

    // get user data
    auto* bind_data   = reinterpret_cast<V8BindStaticFieldData*>(info.Data().As<External>()->Value());
    auto* skr_isolate = reinterpret_cast<V8Isolate*>(Isolate->GetData(0));

    // return field
    Local<Value> result;
    if (V8BindTools::native_to_v8_primitive(
        Context,
        Isolate,
        bind_data->field->type,
        bind_data->field->address,
        result
    ))
    {
        info.GetReturnValue().Set(result);
    }
    else if (V8BindTools::native_to_v8_box(
        Context,
        Isolate,
        bind_data->field->type,
        bind_data->field->address,
        result
    ))
    {
        info.GetReturnValue().Set(result);
    }
    else
    {
        Isolate->ThrowError("field type not supported");
    }
}
void V8Isolate::_set_static_field(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    // get data
    Isolate*       Isolate = info.GetIsolate();
    Local<Context> Context = Isolate->GetCurrentContext();

    // scopes
    Isolate::Scope IsolateScope(Isolate);
    HandleScope    HandleScope(Isolate);
    Context::Scope ContextScope(Context);

    // get user data
    auto* bind_data   = reinterpret_cast<V8BindStaticFieldData*>(info.Data().As<External>()->Value());
    auto* skr_isolate = reinterpret_cast<V8Isolate*>(Isolate->GetData(0));

    // set field
    if (V8BindTools::v8_to_native_primitive(
        Context,
        Isolate,
        bind_data->field->type,
        info[0],
        bind_data->field->address,
        true
    ))
    {
    }
    else if (V8BindTools::v8_to_native_box(
        Context,
        Isolate,
        bind_data->field->type,
        info[0],
        bind_data->field->address,
        true
    ))
    {
    }
    else
    {
        Isolate->ThrowError("field type not supported");
    }
}
} // namespace skr

namespace skr
{
static auto& _v8_platform()
{
    static auto _platform = ::v8::platform::NewDefaultPlatform();
    return _platform;
}

void init_v8()
{
    // init flags
    char Flags[] = "--expose-gc";
    ::v8::V8::SetFlagsFromString(Flags, sizeof(Flags));

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
} // namespace skr