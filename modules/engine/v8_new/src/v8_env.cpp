#include <SkrV8/v8_env.hpp>
#include <SkrContainers/set.hpp>
#include <SkrCore/log.hpp>
#include <SkrRTTR/type.hpp>
#include <SkrV8/v8_bind.hpp>
#include <SkrV8/v8_bind_proxy.hpp>
#include <SkrV8/bind_template/v8_bind_template.hpp>
#include <SkrV8/bind_template/v8bt_primitive.hpp>
#include <SkrV8/bind_template/v8bt_enum.hpp>
#include <SkrV8/bind_template/v8bt_mapping.hpp>
#include <SkrV8/bind_template/v8bt_object.hpp>
#include <SkrV8/bind_template/v8bt_value.hpp>


// v8 includes
#include <libplatform/libplatform.h>
#include <v8-initialization.h>
#include <v8-template.h>
#include <v8-external.h>
#include <v8-function.h>
#include <v8-container.h>

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
//============================isolate============================
// ctor & dtor
V8Isolate::V8Isolate()
{
}
V8Isolate::~V8Isolate()
{
    if (is_init())
    {
        shutdown();
    }
}

// init & shutdown
void V8Isolate::init()
{
    using namespace ::v8;
    SKR_ASSERT(!is_init());

    // init isolate
    _isolate_create_params.array_buffer_allocator = SkrNew<V8Allocator>();
    _isolate                                      = Isolate::New(_isolate_create_params);
    _isolate->SetData(0, this);

    // auto microtasks
    _isolate->SetMicrotasksPolicy(v8::MicrotasksPolicy::kAuto);

    // TODO. dynamic module support
    // _isolate->SetHostImportModuleDynamicallyCallback(_dynamic_import_module); // used for support module
    // _isolate->SetHostInitializeImportMetaObjectCallback(); // used for set import.meta
    // v8::Module::CreateSyntheticModule

    // TODO. promise support
    // _isolate->SetPromiseRejectCallback; // used for capture unhandledRejection

    // TODO. init main context
}
void V8Isolate::shutdown()
{
    SKR_ASSERT(is_init());

    // dispose isolate
    _isolate->Dispose();
    _isolate = nullptr;
}
bool V8Isolate::is_init() const
{
    return _isolate != nullptr;
}

// isolate operators
void V8Isolate::pump_message_loop()
{
    while (v8::platform::PumpMessageLoop(&get_v8_platform(), _isolate)) {};
}
void V8Isolate::gc(bool full)
{
    _isolate->RequestGarbageCollectionForTesting(full ? ::v8::Isolate::kFullGarbageCollection : ::v8::Isolate::kMinorGarbageCollection);

    _isolate->LowMemoryNotification();
    _isolate->IdleNotificationDeadline(0);
}

//==> IV8BindManager API
// bind proxy management
V8BTGeneric* V8Isolate::solve_bind_tp(
    TypeSignatureView signature
)
{
    SKR_UNIMPLEMENTED_FUNCTION();
    return nullptr;
}
V8BindTemplate* V8Isolate::solve_bind_tp(
    const GUID& type_id
)
{
    // find exist bind template
    if (auto found = _bind_tp_map.find(type_id))
    {
        return found.value();
    }

    // get binder
    auto binder = _tmp_binder_mgr.get_or_build(type_id);

    // create bind template
    switch (binder.kind())
    {
    case ScriptBinderRoot::EKind::Primitive: {
        auto bind_tp = SkrNew<V8BTPrimitive>();
        bind_tp->set_manager(this);
        bind_tp->setup(*binder.primitive());
        _bind_tp_map.add(type_id, bind_tp);
        return bind_tp;
    }
    case ScriptBinderRoot::EKind::Mapping: {
        auto bind_tp = SkrNew<V8BTMapping>();
        bind_tp->set_manager(this);
        bind_tp->setup(*binder.mapping());
        _bind_tp_map.add(type_id, bind_tp);
        return bind_tp;
    }
    case ScriptBinderRoot::EKind::Enum: {
        auto bind_tp = SkrNew<V8BTEnum>();
        bind_tp->set_manager(this);
        bind_tp->setup(*binder.enum_());
        _bind_tp_map.add(type_id, bind_tp);
        return bind_tp;
    }
    case ScriptBinderRoot::EKind::Value: {
        auto bind_tp = SkrNew<V8BTValue>();
        bind_tp->set_manager(this);
        bind_tp->setup(*binder.value());
        _bind_tp_map.add(type_id, bind_tp);
        return bind_tp;
    }
    case ScriptBinderRoot::EKind::Object: {
        auto bind_tp = SkrNew<V8BTObject>();
        bind_tp->set_manager(this);
        bind_tp->setup(*binder.object());
        _bind_tp_map.add(type_id, bind_tp);
        return bind_tp;
    }
    default:
        SKR_UNREACHABLE_CODE();
        return nullptr;
    }
}

// bind proxy management
void V8Isolate::add_bind_proxy(
    void*        native_ptr,
    V8BindProxy* bind_proxy
)
{
    _bind_proxy_map.add(native_ptr, bind_proxy);
}
void V8Isolate::remove_bind_proxy(
    void*        native_ptr,
    V8BindProxy* bind_proxy
)
{
    _bind_proxy_map.remove(native_ptr);
}
V8BindProxy* V8Isolate::find_bind_proxy(
    void* native_ptr
) const
{
    if (auto found = _bind_proxy_map.find(native_ptr))
    {
        return found.value();
    }
    return nullptr;
}

// 用于缓存调用期间为参数创建的临时 bind proxy
void V8Isolate::push_param_scope()
{
    SKR_UNIMPLEMENTED_FUNCTION()
}
void V8Isolate::pop_param_scope()
{
    SKR_UNIMPLEMENTED_FUNCTION()
}
void V8Isolate::push_param_proxy(
    V8BindProxy* bind_proxy
){
    SKR_UNIMPLEMENTED_FUNCTION()
}

// mixin
IScriptMixinCore* V8Isolate::get_mixin_core() const
{
    return const_cast<V8Isolate*>(this);
}
//==> IV8BindManager API

//============================global init============================
static auto& _v8_platform()
{
    static v8::Platform* _platform = nullptr;
    return _platform;
}
void init_v8()
{
    // init flags
    char Flags[] = "--expose-gc";
    ::v8::V8::SetFlagsFromString(Flags, sizeof(Flags));

    // init platform
    _v8_platform() = ::v8::platform::NewDefaultPlatform_Without_Stl();
    ::v8::V8::InitializePlatform(_v8_platform());

    // init v8
    ::v8::V8::Initialize();
}
void shutdown_v8()
{
    // shutdown v8
    ::v8::V8::Dispose();

    // shutdown platform
    ::v8::V8::DisposePlatform();
    delete _v8_platform();
}
v8::Platform& get_v8_platform()
{
    return *_v8_platform();
}
} // namespace skr