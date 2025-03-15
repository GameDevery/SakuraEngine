#include "SkrV8/v8_isolate.hpp"
#include "SkrContainers/set.hpp"
#include "SkrCore/log.hpp"
#include "SkrV8/v8_bind_data.hpp"
#include "libplatform/libplatform.h"
#include "v8-initialization.h"
#include "SkrRTTR/type.hpp"
#include "v8-template.h"
#include "v8-external.h"
#include "v8-function.h"
#include "SkrV8/v8_bind.hpp"

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
            if (bind_core->object->script_owner_ship() == EScriptbleObjectOwnerShip::Script)
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
void V8Isolate::make_record_template(::skr::RTTRType* type)
{
    using namespace ::v8;

    // check
    SKR_ASSERT(type->type_category() == ::skr::ERTTRTypeCategory::Record);
    SKR_ASSERT(type->based_on(type_id_of<ScriptbleObject>()));

    // find exist template
    if (_record_templates.contains(type))
    {
        return;
    }

    // v8 scope
    Isolate::Scope isolate_scope(_isolate);
    HandleScope    handle_scope(_isolate);

    // new bind data
    auto bind_data = SkrNew<V8BindWrapData>();

    // ctor template
    auto ctor_template = FunctionTemplate::New(
        _isolate,
        _call_ctor,
        External::New(_isolate, bind_data)
    );
    bind_data->ctor_template.Reset(_isolate, ctor_template);

    // setup internal field count
    ctor_template->InstanceTemplate()->SetInternalFieldCount(1);

    // solve binder info
    ScriptBinderRoot  binder      = _binder_mgr.get_or_build(type->type_id());
    ScriptBinderWrap* wrap_binder = binder.wrap();
    bind_data->binder             = wrap_binder;

    // bind method
    for (const auto& [method_name, method_binder] : wrap_binder->methods)
    {
        auto method_bind_data    = SkrNew<V8BindMethodData>();
        method_bind_data->binder = method_binder;
        ctor_template->PrototypeTemplate()->Set(
            V8Bind::to_v8(method_name, true),
            FunctionTemplate::New(
                _isolate,
                _call_method,
                External::New(_isolate, method_bind_data)
            )
        );
    }

    // bind static method
    for (const auto& [static_method_name, static_method_binder] : wrap_binder->static_methods)
    {
        auto static_method_bind_data    = SkrNew<V8BindStaticMethodData>();
        static_method_bind_data->binder = static_method_binder;
        ctor_template->Set(
            V8Bind::to_v8(static_method_name, true),
            FunctionTemplate::New(
                _isolate,
                _call_static_method,
                External::New(_isolate, static_method_bind_data)
            )
        );
    }

    // bind field
    for (const auto& [field_name, field_binder] : wrap_binder->fields)
    {
        auto field_bind_data    = SkrNew<V8BindFieldData>();
        field_bind_data->binder = field_binder;
        ctor_template->PrototypeTemplate()->SetAccessorProperty(
            V8Bind::to_v8(field_name, true),
            FunctionTemplate::New(
                _isolate,
                _get_field,
                External::New(_isolate, field_bind_data)
            ),
            FunctionTemplate::New(
                _isolate,
                _set_field,
                External::New(_isolate, field_bind_data)
            )
        );
    }

    // bind static field
    for (const auto& [static_field_name, static_field_binder] : wrap_binder->static_fields)
    {
        auto static_field_bind_data    = SkrNew<V8BindStaticFieldData>();
        static_field_bind_data->binder = static_field_binder;
        ctor_template->SetAccessorProperty(
            V8Bind::to_v8(static_field_name, true),
            FunctionTemplate::New(
                _isolate,
                _get_static_field,
                External::New(_isolate, static_field_bind_data)
            ),
            FunctionTemplate::New(
                _isolate,
                _set_static_field,
                External::New(_isolate, static_field_bind_data)
            )
        );
    }

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
                                   V8Bind::to_v8(type->name(), true),
                                   function
        )
            .Check();
    }
}

// bind object
V8BindRecordCore* V8Isolate::translate_record(::skr::ScriptbleObject* obj)
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
    auto type = get_type_from_guid(obj->iobject_get_typeid());

    // get template
    auto template_ref = _record_templates.find(type);
    if (!template_ref)
    {
        return nullptr;
    }

    // make object
    Local<Function> ctor_func = template_ref.value()->ctor_template.Get(_isolate)->GetFunction(context).ToLocalChecked();
    Local<Object>   object    = ctor_func->NewInstance(context).ToLocalChecked();

    // make bind data
    auto bind_data    = SkrNew<V8BindRecordCore>();
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
void V8Isolate::mark_record_deleted(::skr::ScriptbleObject* obj)
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
    V8Isolate*        isolate   = reinterpret_cast<V8Isolate*>(data.GetIsolate()->GetData(0));

    // remove alive object
    if (bind_core->object)
    {
        // delete if has owner ship
        if (bind_core->object->script_owner_ship() == EScriptbleObjectOwnerShip::Script)
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
    auto* bind_data   = reinterpret_cast<V8BindWrapData*>(info.Data().As<External>()->Value());
    auto* skr_isolate = reinterpret_cast<V8Isolate*>(Isolate->GetData(0));

    // handle call
    if (info.IsConstructCall())
    {
        // get ctor info
        Local<Object> self = info.This();

        // check constructable
        if (!bind_data->binder->is_script_newable)
        {
            Isolate->ThrowError("record is not constructable");
            return;
        }

        // match ctor
        for (const auto& ctor_binder : bind_data->binder->ctors)
        {
            if (!V8Bind::match(ctor_binder.params_binder, info)) continue;

            // alloc memory
            void* alloc_mem = sakura_new_aligned(bind_data->binder->type->size(), bind_data->binder->type->alignment());

            // call ctor
            V8Bind::call_native(
                ctor_binder,
                info,
                alloc_mem
            );

            // cast to ScriptbleObject
            void* casted_mem = bind_data->binder->type->cast_to_base(type_id_of<ScriptbleObject>(), alloc_mem);

            // make bind core
            V8BindRecordCore* bind_core = SkrNew<V8BindRecordCore>();
            bind_core->type             = bind_data->binder->type;
            bind_core->object           = reinterpret_cast<ScriptbleObject*>(casted_mem);

            // setup owner ship
            bind_core->object->script_owner_ship_take(EScriptbleObjectOwnerShip::Script);

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

            return;
        }

        // no ctor called
        Isolate->ThrowError("no ctor matched");
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

    // call method
    bool success = V8Bind::call_native(
        bind_data->binder,
        info,
        bind_core->object->iobject_get_head_ptr(),
        bind_core->type
    );

    // throw
    if (!success)
    {
        Isolate->ThrowError("no matched method");
        return;
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

    // call method
    bool success = V8Bind::call_native(
        bind_data->binder,
        info
    );

    // throw
    if (!success)
    {
        Isolate->ThrowError("no matched method");
        return;
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

    // get field
    auto v8_field = V8Bind::get_field(
        bind_data->binder,
        bind_core->object->iobject_get_head_ptr(),
        bind_core->type
    );
    info.GetReturnValue().Set(v8_field);
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

    // set field
    V8Bind::set_field(
        bind_data->binder,
        info[0],
        bind_core->object->iobject_get_head_ptr(),
        bind_core->type
    );
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

    // get field
    auto v8_field = V8Bind::get_field(
        bind_data->binder
    );
    info.GetReturnValue().Set(v8_field);
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
    V8Bind::set_field(
        bind_data->binder,
        info[0]
    );
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