#include <SkrV8/v8_bind_manager.hpp>
#include "v8-template.h"
#include "v8-external.h"
#include <SkrV8/v8_bind.hpp>

namespace skr
{
// ctor & dtor
V8BindManager::V8BindManager()
{
}
V8BindManager::~V8BindManager()
{
    cleanup_templates();
    cleanup_bind_cores();
}

// init & shutdown
void V8BindManager::cleanup_templates()
{
    for (auto& [type, bind_data] : _record_templates)
    {
        bind_data->ctor_template.Reset();
        SkrDelete(bind_data);
    }
    _record_templates.clear();
    for (auto& [type, bind_data] : _enum_templates)
    {
        bind_data->enum_template.Reset();
        SkrDelete(bind_data);
    }
    _enum_templates.clear();
}
void V8BindManager::cleanup_bind_cores()
{
    for (auto& [obj, bind_core] : _alive_objects)
    {
        // delete object if only script owns it
        if (bind_core->object->ownership() == EScriptbleObjectOwnership::Script)
        {
            SkrDelete(bind_core->object);
        }

        // delete core
        SkrDelete(bind_core);
    }
    _alive_objects.clear();
    for (auto& bind_core : _deleted_objects)
    {
        // just delete core
        SkrDelete(bind_core);
    }
}

// bind object
V8BindCoreObject* V8BindManager::translate_object(::skr::ScriptbleObject* obj)
{
    using namespace ::v8;

    // get v8 context
    auto* isolate = Isolate::GetCurrent();
    auto  context = isolate->GetCurrentContext();

    // make scope
    HandleScope handle_scope(isolate);

    // find exist object
    if (auto found = _alive_objects.find(obj))
    {
        return found.value();
    }

    // get type
    auto type = get_type_from_guid(obj->iobject_get_typeid());

    // get template
    auto found_template = _record_templates.find(type);
    if (!found_template)
    {
        SKR_LOG_FMT_ERROR(
            u8"type {} template not found, please register it first",
            type->name()
        );
        return nullptr;
    }

    // make object
    Local<ObjectTemplate> instance_template = found_template.value()->ctor_template.Get(isolate)->InstanceTemplate();
    Local<Object>         object            = instance_template->NewInstance(context).ToLocalChecked();

    // make bind core
    auto object_bind_core  = _new_bind_data<V8BindCoreObject>();
    object_bind_core->type = type;
    object_bind_core->data = obj->iobject_get_head_ptr();
    object_bind_core->v8_object.Reset(isolate, object);
    object_bind_core->object = obj;

    // setup gc callback
    object_bind_core->v8_object.SetWeak(
        (V8BindCoreRecordBase*)object_bind_core,
        _gc_callback,
        WeakCallbackType::kInternalFields
    );

    // add extern data
    object->SetInternalField(0, External::New(isolate, object_bind_core));

    // add to map
    _alive_objects.add(obj, object_bind_core);

    // bind mixin core
    obj->set_mixin_core(this);

    return object_bind_core;
}
void V8BindManager::mark_object_deleted(::skr::ScriptbleObject* obj)
{
    if (auto found = _alive_objects.find(obj))
    {
        // reset core object ptr
        found.value()->invalidate();

        // move to deleted objects
        _deleted_objects.push_back(found.value());
        _alive_objects.remove(obj);
    }
}

// bind value
V8BindCoreValue* V8BindManager::create_value(const RTTRType* type, const void* source_data)
{
    using namespace ::v8;

    auto* isolate = Isolate::GetCurrent();
    auto  context = isolate->GetCurrentContext();

    HandleScope handle_scope(isolate);

    // get template
    auto template_ref = _record_templates.find(type);
    if (!template_ref)
    {
        return nullptr;
    }

    void* alloc_mem;

    if (source_data)
    {
        // find copy ctor
        TypeSignatureBuilder tb;
        tb.write_function_signature(1);
        tb.write_type_id(type_id_of<void>()); // return
        tb.write_const();
        tb.write_ref();
        tb.write_type_id(type->type_id()); // param 1
        auto* found_copy_ctor = type->find_ctor({ .signature = tb.type_signature_view() });
        if (!found_copy_ctor)
        {
            return nullptr;
        }

        // alloc value
        alloc_mem = sakura_new_aligned(type->size(), type->alignment());

        // call copy ctor
        auto* invoker = reinterpret_cast<void (*)(void*, const void*)>(found_copy_ctor->native_invoke);
        invoker(alloc_mem, source_data);
    }
    else
    {
        // find default ctor
        auto* found_default_ctor = type->find_ctor_t<void()>();
        if (!found_default_ctor)
        {
            return nullptr;
        }

        // alloc value
        alloc_mem = sakura_new_aligned(type->size(), type->alignment());

        // call default ctor
        auto* invoker = reinterpret_cast<void (*)(void*)>(found_default_ctor->native_invoke);
        invoker(alloc_mem);
    }

    // make object
    Local<ObjectTemplate> instance_template = template_ref.value()->ctor_template.Get(isolate)->InstanceTemplate();
    Local<Object>         object            = instance_template->NewInstance(context).ToLocalChecked();

    // make bind core
    auto value_bind_core     = SkrNew<V8BindCoreValue>();
    value_bind_core->manager = this;
    value_bind_core->type    = type;
    value_bind_core->data    = alloc_mem;
    value_bind_core->v8_object.Reset(isolate, object);

    // setup source
    value_bind_core->from = V8BindCoreValue::ESource::Create;

    // setup gc callback
    value_bind_core->v8_object.SetWeak(
        (V8BindCoreRecordBase*)value_bind_core,
        _gc_callback,
        WeakCallbackType::kInternalFields
    );

    // add extern data
    object->SetInternalField(0, External::New(isolate, value_bind_core));

    // add to map
    _script_created_values.add(alloc_mem, value_bind_core);

    return value_bind_core;
}
V8BindCoreValue* V8BindManager::translate_value_field(const RTTRType* type, const void* data, V8BindCoreRecordBase* owner)
{
    using namespace ::v8;

    auto* isolate = Isolate::GetCurrent();
    auto  context = isolate->GetCurrentContext();

    HandleScope handle_scope(isolate);

    // get template
    auto template_ref = _record_templates.find(type);
    if (!template_ref)
    {
        return nullptr;
    }

    // find exported data
    auto found_data = owner->cache_value_fields.find(data);
    if (found_data)
    {
        return found_data.value();
    }

    // make object
    Local<ObjectTemplate> instance_template = template_ref.value()->ctor_template.Get(isolate)->InstanceTemplate();
    Local<Object>         object            = instance_template->NewInstance(context).ToLocalChecked();

    // make bind core
    auto value_bind_core     = SkrNew<V8BindCoreValue>();
    value_bind_core->manager = this;
    value_bind_core->type    = type;
    value_bind_core->data    = const_cast<void*>(data);
    value_bind_core->v8_object.Reset(isolate, object);

    // setup source
    value_bind_core->from             = V8BindCoreValue::ESource::Field;
    value_bind_core->from_field_owner = owner;

    // setup gc callback
    value_bind_core->v8_object.SetWeak(
        (V8BindCoreRecordBase*)value_bind_core,
        _gc_callback,
        WeakCallbackType::kInternalFields
    );

    // add extern data
    object->SetInternalField(0, External::New(isolate, value_bind_core));

    // add to map
    owner->cache_value_fields.add(const_cast<void*>(data), value_bind_core);

    return value_bind_core;
}

// query template
v8::Local<v8::ObjectTemplate> V8BindManager::get_enum_template(const RTTRType* type)
{
    using namespace ::v8;
    // check
    SKR_ASSERT(type->is_enum());

    auto* isolate = Isolate::GetCurrent();

    // v8 scope
    EscapableHandleScope handle_scope(isolate);

    // find exists template
    if (auto result = _enum_templates.find(type))
    {
        return handle_scope.Escape(result.value()->enum_template.Get(isolate));
    }

    // get binder
    ScriptBinderRoot  binder      = _binder_mgr.get_or_build(type->type_id());
    ScriptBinderEnum* enum_binder = binder.enum_();

    // new bind data
    auto bind_data    = _new_bind_data<V8BindDataEnum>();
    bind_data->binder = binder.enum_();

    // object template
    auto object_template = ObjectTemplate::New(isolate);

    // add enum items
    for (const auto& [enum_item_name, enum_item] : enum_binder->items)
    {
        // get value
        Local<Value> enum_value = V8Bind::to_v8(enum_item->value);

        // set value
        object_template->Set(
            V8Bind::to_v8(enum_item->name, true),
            enum_value
        );
    }

    // add convert functions
    object_template->Set(
        V8Bind::to_v8(u8"to_string", true),
        FunctionTemplate::New(
            isolate,
            _enum_to_string,
            External::New(isolate, bind_data)
        )
    );
    object_template->Set(
        V8Bind::to_v8(u8"from_string", true),
        FunctionTemplate::New(
            isolate,
            _enum_from_string,
            External::New(isolate, bind_data)
        )
    );

    _enum_templates.add(type, bind_data);
    return handle_scope.Escape(object_template);
}
v8::Local<v8::FunctionTemplate> V8BindManager::get_record_template(const RTTRType* type)
{
    using namespace ::v8;

    // check
    SKR_ASSERT(type->is_record());

    auto* isolate = Isolate::GetCurrent();

    // v8 scope
    EscapableHandleScope handle_scope(isolate);

    // find exists template
    if (auto result = _record_templates.find(type))
    {
        return handle_scope.Escape(result.value()->ctor_template.Get(isolate));
    }

    // get binder
    ScriptBinderRoot binder = _binder_mgr.get_or_build(type->type_id());
    if (binder.is_object())
    {
        return handle_scope.Escape(_make_template_object(binder.object()));
    }
    else if (binder.is_value())
    {
        return handle_scope.Escape(_make_template_value(binder.value()));
    }
    else
    {
        return {};
    }
}

// => IScriptMixinCore API
void V8BindManager::on_object_destroyed(ScriptbleObject* obj)
{
    mark_object_deleted(obj);
}

// template helper
v8::Local<v8::FunctionTemplate> V8BindManager::_make_template_object(ScriptBinderObject* object_binder)
{
    using namespace ::v8;

    auto* isolate = Isolate::GetCurrent();

    // v8 scope
    EscapableHandleScope handle_scope(isolate);

    // new bind data
    auto bind_data    = _new_bind_data<V8BindDataObject>();
    bind_data->binder = object_binder;

    // ctor template
    auto ctor_template = FunctionTemplate::New(
        isolate,
        _call_ctor,
        External::New(isolate, bind_data)
    );
    bind_data->ctor_template.Reset(isolate, ctor_template);

    // fill template
    _fill_record_template(object_binder, bind_data, ctor_template);

    // store template
    _record_templates.add(object_binder->type, bind_data);

    return handle_scope.Escape(ctor_template);
}
v8::Local<v8::FunctionTemplate> V8BindManager::_make_template_value(ScriptBinderValue* value_binder)
{
    using namespace ::v8;

    auto* isolate = Isolate::GetCurrent();

    // v8 scope
    EscapableHandleScope handle_scope(isolate);

    // new bind data
    auto bind_data    = _new_bind_data<V8BindDataValue>();
    bind_data->binder = value_binder;

    // ctor template
    auto ctor_template = FunctionTemplate::New(
        isolate,
        _call_ctor,
        External::New(isolate, bind_data)
    );
    bind_data->ctor_template.Reset(isolate, ctor_template);

    // fill template
    _fill_record_template(value_binder, bind_data, ctor_template);

    // store template
    _record_templates.add(value_binder->type, bind_data);

    return handle_scope.Escape(ctor_template);
}

void V8BindManager::_fill_record_template(
    ScriptBinderRecordBase*         binder,
    V8BindDataRecordBase*           bind_data,
    v8::Local<v8::FunctionTemplate> ctor_template
)
{
    using namespace ::v8;

    auto* isolate = Isolate::GetCurrent();

    // setup internal field count
    ctor_template->InstanceTemplate()->SetInternalFieldCount(1);

    // bind method
    for (const auto& [method_name, method_binder] : binder->methods)
    {
        auto method_bind_data    = _new_bind_data<V8BindDataMethod>();
        method_bind_data->binder = method_binder;
        ctor_template->PrototypeTemplate()->Set(
            V8Bind::to_v8(method_name, true),
            FunctionTemplate::New(
                isolate,
                _call_method,
                External::New(isolate, method_bind_data)
            )
        );
        bind_data->methods.add(method_name, method_bind_data);
    }

    // bind static method
    for (const auto& [static_method_name, static_method_binder] : binder->static_methods)
    {
        auto static_method_bind_data    = _new_bind_data<V8BindDataStaticMethod>();
        static_method_bind_data->binder = static_method_binder;
        ctor_template->Set(
            V8Bind::to_v8(static_method_name, true),
            FunctionTemplate::New(
                isolate,
                _call_static_method,
                External::New(isolate, static_method_bind_data)
            )
        );
        bind_data->static_methods.add(static_method_name, static_method_bind_data);
    }

    // bind field
    for (const auto& [field_name, field_binder] : binder->fields)
    {
        auto field_bind_data    = _new_bind_data<V8BindDataField>();
        field_bind_data->binder = field_binder;
        ctor_template->PrototypeTemplate()->SetAccessorProperty(
            V8Bind::to_v8(field_name, true),
            FunctionTemplate::New(
                isolate,
                _get_field,
                External::New(isolate, field_bind_data)
            ),
            FunctionTemplate::New(
                isolate,
                _set_field,
                External::New(isolate, field_bind_data)
            )
        );
        bind_data->fields.add(field_name, field_bind_data);
    }

    // bind static field
    for (const auto& [static_field_name, static_field_binder] : binder->static_fields)
    {
        auto static_field_bind_data    = _new_bind_data<V8BindDataStaticField>();
        static_field_bind_data->binder = static_field_binder;
        ctor_template->SetAccessorProperty(
            V8Bind::to_v8(static_field_name, true),
            FunctionTemplate::New(
                isolate,
                _get_static_field,
                External::New(isolate, static_field_bind_data)
            ),
            FunctionTemplate::New(
                isolate,
                _set_static_field,
                External::New(isolate, static_field_bind_data)
            )
        );
        bind_data->static_fields.add(static_field_name, static_field_bind_data);
    }

    // bind properties
    for (const auto& [property_name, property_binder] : binder->properties)
    {
        auto property_bind_data    = _new_bind_data<V8BindDataProperty>();
        property_bind_data->binder = property_binder;
        ctor_template->PrototypeTemplate()->SetAccessorProperty(
            V8Bind::to_v8(property_name, true),
            FunctionTemplate::New(
                isolate,
                _get_prop,
                External::New(isolate, property_bind_data)
            ),
            FunctionTemplate::New(
                isolate,
                _set_prop,
                External::New(isolate, property_bind_data)
            )
        );
        bind_data->properties.add(property_name, property_bind_data);
    }

    // bind static properties
    for (const auto& [static_property_name, static_property_binder] : binder->static_properties)
    {
        auto static_property_bind_data    = _new_bind_data<V8BindDataStaticProperty>();
        static_property_bind_data->binder = static_property_binder;
        ctor_template->SetAccessorProperty(
            V8Bind::to_v8(static_property_name, true),
            FunctionTemplate::New(
                isolate,
                _get_static_prop,
                External::New(isolate, static_property_bind_data)
            ),
            FunctionTemplate::New(
                isolate,
                _set_static_prop,
                External::New(isolate, static_property_bind_data)
            )
        );
        bind_data->static_properties.add(static_property_name, static_property_bind_data);
    }
}

// bind methods
void V8BindManager::_gc_callback(const ::v8::WeakCallbackInfo<V8BindCoreRecordBase>& data)
{
    using namespace ::v8;

    V8BindCoreRecordBase* bind_core = data.GetParameter();

    if (bind_core->is_value)
    {
        auto* value_core = bind_core->as_value();

        // do release if not field export case
        if (value_core->from == V8BindCoreValue::ESource::Create)
        {
            // call destructor
            auto dtor_data = value_core->type->dtor_data();
            if (dtor_data)
            {
                dtor_data.value().native_invoke(value_core->data);
            }

            // release data
            sakura_free_aligned(value_core->data, value_core->type->alignment());

            // reset object handle
            value_core->v8_object.Reset();

            // remove from map
            value_core->manager->_script_created_values.remove(value_core->data);

            // release core
            SkrDelete(value_core);
        }
        else
        {
            if (value_core->from == V8BindCoreValue::ESource::Field)
            {
                // remove from cache
                value_core->from_field_owner->cache_value_fields.remove(value_core->data);
            }
            else if (value_core->from == V8BindCoreValue::ESource::StaticField)
            {
                // TODO. static field
            }

            // reset object handle
            value_core->v8_object.Reset();

            // release core
            SkrDelete(value_core);
        }
    }
    else
    {
        auto* object_core = bind_core->as_object();

        // get data
        auto* bind_manager = bind_core->manager;

        // remove alive object
        if (object_core->object)
        {
            // delete if has owner ship
            if (object_core->object->ownership() == EScriptbleObjectOwnership::Script)
            {
                SkrDelete(object_core->object);
            }

            bind_manager->_alive_objects.remove(object_core->object);
        }
        else
        {
            // remove from deleted records
            bind_manager->_deleted_objects.remove(bind_core);
        }

        // reset object handle
        object_core->v8_object.Reset();

        // release core
        SkrDelete(object_core);
    }
}
void V8BindManager::_call_ctor(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    // get v8 basic info
    Isolate* Isolate = info.GetIsolate();

    // scopes
    HandleScope HandleScope(Isolate);

    // get user data
    auto* bind_data    = reinterpret_cast<V8BindDataRecordBase*>(info.Data().As<External>()->Value());
    auto* bind_manager = bind_data->manager;

    // handle call
    if (info.IsConstructCall())
    {
        // get ctor info
        Local<Object> self = info.This();

        if (bind_data->is_value)
        {
            auto* value_data = bind_data->as_value();
            auto* binder     = value_data->binder;

            // check constructable
            if (!binder->is_script_newable)
            {
                Isolate->ThrowError("record is not constructable");
                return;
            }

            // match ctor
            for (const auto& ctor_binder : binder->ctors)
            {
                if (!V8Bind::match(ctor_binder.params_binder, ctor_binder.params_binder.size(), info)) continue;

                // alloc memory
                void* alloc_mem = sakura_new_aligned(binder->type->size(), binder->type->alignment());

                // call ctor
                V8Bind::call_native(
                    ctor_binder,
                    info,
                    alloc_mem
                );

                // make bind core
                V8BindCoreValue* bind_core = SkrNew<V8BindCoreValue>();
                bind_core->manager         = bind_data->manager;
                bind_core->type            = binder->type;
                bind_core->data            = alloc_mem;

                // setup source
                bind_core->from = V8BindCoreValue::ESource::Create;

                // setup gc callback
                bind_core->v8_object.Reset(Isolate, self);
                bind_core->v8_object.SetWeak(
                    (V8BindCoreRecordBase*)bind_core,
                    _gc_callback,
                    WeakCallbackType::kInternalFields
                );

                // add extern data
                self->SetInternalField(0, External::New(Isolate, bind_core));

                // add to map
                bind_manager->_script_created_values.add(bind_core->data, bind_core);

                return;
            }

            // no ctor called
            Isolate->ThrowError("no ctor matched");
        }
        else
        {
            auto* object_data = bind_data->as_object();
            auto* binder      = object_data->binder;

            // check constructable
            if (!binder->is_script_newable)
            {
                Isolate->ThrowError("record is not constructable");
                return;
            }

            // match ctor
            for (const auto& ctor_binder : binder->ctors)
            {
                if (!V8Bind::match(ctor_binder.params_binder, ctor_binder.params_binder.size(), info)) continue;

                // alloc memory
                void* alloc_mem = sakura_new_aligned(binder->type->size(), binder->type->alignment());

                // call ctor
                V8Bind::call_native(
                    ctor_binder,
                    info,
                    alloc_mem
                );

                // cast to ScriptbleObject
                void* casted_mem = binder->type->cast_to_base(type_id_of<ScriptbleObject>(), alloc_mem);

                // make bind core
                V8BindCoreObject* bind_core = SkrNew<V8BindCoreObject>();
                bind_core->manager          = bind_data->manager;
                bind_core->type             = binder->type;
                bind_core->data             = alloc_mem;
                bind_core->object           = reinterpret_cast<ScriptbleObject*>(casted_mem);

                // setup owner ship
                bind_core->object->ownership_take_script();

                // setup gc callback
                bind_core->v8_object.Reset(Isolate, self);
                bind_core->v8_object.SetWeak(
                    (V8BindCoreRecordBase*)bind_core,
                    _gc_callback,
                    WeakCallbackType::kInternalFields
                );

                // add extern data
                self->SetInternalField(0, External::New(Isolate, bind_core));

                // add to map
                bind_manager->_alive_objects.add(bind_core->object, bind_core);

                return;
            }

            // no ctor called
            Isolate->ThrowError("no ctor matched");
        }
    }
    else
    {
        Isolate->ThrowError("must be called with new");
    }
}
void V8BindManager::_call_method(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    // get v8 basic info
    Isolate* Isolate = info.GetIsolate();

    // scopes
    HandleScope HandleScope(Isolate);

    // get self data
    Local<Object> self      = info.This();
    auto*         bind_core = reinterpret_cast<V8BindCoreRecordBase*>(self->GetInternalField(0).As<External>()->Value());

    // check core
    if (!bind_core->is_valid())
    {
        Isolate->ThrowError("calling on a released object");
        return;
    }

    // get user data
    auto* bind_data = reinterpret_cast<V8BindDataMethod*>(info.Data().As<External>()->Value());

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
        bind_core->data,
        bind_core->type
    );

    // throw
    if (!success)
    {
        Isolate->ThrowError("no matched method");
        return;
    }
}
void V8BindManager::_call_static_method(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    // get v8 basic info
    Isolate* Isolate = info.GetIsolate();

    // scopes
    HandleScope HandleScope(Isolate);

    // get user data
    auto* bind_data = reinterpret_cast<V8BindDataStaticMethod*>(info.Data().As<External>()->Value());

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
void V8BindManager::_get_field(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    // get data
    Isolate* Isolate = info.GetIsolate();

    // scopes
    HandleScope HandleScope(Isolate);

    // get self data
    Local<Object> self      = info.This();
    auto*         bind_core = reinterpret_cast<V8BindCoreRecordBase*>(self->GetInternalField(0).As<External>()->Value());

    // check core
    if (!bind_core->is_valid())
    {
        Isolate->ThrowError("calling on a released object");
        return;
    }

    // get user data
    auto* bind_data = reinterpret_cast<V8BindDataField*>(info.Data().As<External>()->Value());

    // get field
    auto v8_field = V8Bind::get_field_value_or_object(
        bind_data->binder,
        bind_core
    );
    info.GetReturnValue().Set(v8_field);
}
void V8BindManager::_set_field(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    // get data
    Isolate* Isolate = info.GetIsolate();

    // scopes
    HandleScope HandleScope(Isolate);

    // get self data
    Local<Object> self      = info.This();
    auto*         bind_core = reinterpret_cast<V8BindCoreRecordBase*>(self->GetInternalField(0).As<External>()->Value());

    // check core
    if (!bind_core->is_valid())
    {
        Isolate->ThrowError("calling on a released object");
        return;
    }

    // get user data
    auto* bind_data = reinterpret_cast<V8BindDataField*>(info.Data().As<External>()->Value());

    // set field
    V8Bind::set_field_value_or_object(
        bind_data->binder,
        info[0],
        bind_core
    );
}
void V8BindManager::_get_static_field(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    // get data
    Isolate* Isolate = info.GetIsolate();

    // scopes
    HandleScope HandleScope(Isolate);

    // get user data
    auto* bind_data = reinterpret_cast<V8BindDataStaticField*>(info.Data().As<External>()->Value());

    // get field
    auto v8_field = V8Bind::get_static_field(
        bind_data->binder
    );
    info.GetReturnValue().Set(v8_field);
}
void V8BindManager::_set_static_field(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    // get data
    Isolate* Isolate = info.GetIsolate();

    // scopes
    HandleScope HandleScope(Isolate);

    // get user data
    auto* bind_data = reinterpret_cast<V8BindDataStaticField*>(info.Data().As<External>()->Value());

    // set field
    V8Bind::set_static_field(
        bind_data->binder,
        info[0]
    );
}
void V8BindManager::_get_prop(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    // get data
    Isolate* Isolate = info.GetIsolate();

    // scopes
    HandleScope HandleScope(Isolate);

    // get self data
    Local<Object> self      = info.This();
    auto*         bind_core = reinterpret_cast<V8BindCoreRecordBase*>(self->GetInternalField(0).As<External>()->Value());

    // check core
    if (!bind_core->is_valid())
    {
        Isolate->ThrowError("calling on a released object");
        return;
    }

    // get user data
    auto* bind_data = reinterpret_cast<V8BindDataProperty*>(info.Data().As<External>()->Value());

    // invoke
    V8Bind::call_native(
        bind_data->binder.getter,
        info,
        bind_core->data,
        bind_core->type
    );
}
void V8BindManager::_set_prop(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    // get data
    Isolate* Isolate = info.GetIsolate();

    // scopes
    HandleScope HandleScope(Isolate);

    // get self data
    Local<Object> self      = info.This();
    auto*         bind_core = reinterpret_cast<V8BindCoreRecordBase*>(self->GetInternalField(0).As<External>()->Value());

    // check core
    if (!bind_core->is_valid())
    {
        Isolate->ThrowError("calling on a released object");
        return;
    }

    // get user data
    auto* bind_data = reinterpret_cast<V8BindDataProperty*>(info.Data().As<External>()->Value());

    // invoke
    V8Bind::call_native(
        bind_data->binder.setter,
        info,
        bind_core->data,
        bind_core->type
    );
}
void V8BindManager::_get_static_prop(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    // get data
    Isolate* Isolate = info.GetIsolate();

    // scopes
    HandleScope HandleScope(Isolate);

    // get user data
    auto* bind_data = reinterpret_cast<V8BindDataStaticProperty*>(info.Data().As<External>()->Value());

    // invoke
    V8Bind::call_native(
        bind_data->binder.getter,
        info
    );
}
void V8BindManager::_set_static_prop(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    // get data
    Isolate* Isolate = info.GetIsolate();

    // scopes
    HandleScope HandleScope(Isolate);

    // get user data
    auto* bind_data = reinterpret_cast<V8BindDataStaticProperty*>(info.Data().As<External>()->Value());

    // invoke
    V8Bind::call_native(
        bind_data->binder.setter,
        info
    );
}
void V8BindManager::_enum_to_string(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    // get data
    Isolate* Isolate = info.GetIsolate();

    // scopes
    HandleScope HandleScope(Isolate);

    // get user data
    auto* bind_data = reinterpret_cast<V8BindDataEnum*>(info.Data().As<External>()->Value());

    // check value
    if (info.Length() != 1)
    {
        Isolate->ThrowError("enum to_string need 1 argument");
        return;
    }

    // get value
    int64_t  enum_singed;
    uint64_t enum_unsigned;
    if (bind_data->binder->is_signed)
    {
        if (!V8Bind::to_native(info[0], enum_singed))
        {
            Isolate->ThrowError("invalid enum value");
            return;
        }
    }
    else
    {
        if (!V8Bind::to_native(info[0], enum_unsigned))
        {
            Isolate->ThrowError("invalid enum value");
            return;
        }
    }

    // find value
    for (const auto& [enum_item_name, enum_item] : bind_data->binder->items)
    {
        if (enum_item->value.is_signed())
        {
            int64_t v = enum_item->value.value_signed();
            if (v == enum_singed)
            {
                info.GetReturnValue().Set(V8Bind::to_v8(enum_item_name, true));
                return;
            }
        }
        else
        {
            uint64_t v = enum_item->value.value_unsigned();
            if (v == enum_unsigned)
            {
                info.GetReturnValue().Set(V8Bind::to_v8(enum_item_name, true));
                return;
            }
        }
    }

    Isolate->ThrowError("no matched enum item");
}
void V8BindManager::_enum_from_string(const ::v8::FunctionCallbackInfo<::v8::Value>& info)
{
    using namespace ::v8;

    // get data
    Isolate* Isolate = info.GetIsolate();

    // scopes
    HandleScope HandleScope(Isolate);

    // get user data
    auto* bind_data = reinterpret_cast<V8BindDataEnum*>(info.Data().As<External>()->Value());

    // check value
    if (info.Length() != 1)
    {
        Isolate->ThrowError("enum to_string need 1 argument");
        return;
    }

    // get item name
    String enum_item_name;
    if (!V8Bind::to_native(info[0], enum_item_name))
    {
        Isolate->ThrowError("invalid enum item name");
        return;
    }

    // find value
    if (auto result = bind_data->binder->items.find(enum_item_name))
    {
        info.GetReturnValue().Set(V8Bind::to_v8(result.value()->value));
    }
    else
    {
        Isolate->ThrowError("no matched enum item");
    }
}
} // namespace skr