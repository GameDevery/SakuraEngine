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
    shutdown();
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
        // cleanup templates
        _bind_manager.cleanup_templates();

        // dispose isolate
        _isolate->Dispose();
        _isolate = nullptr;

        // cleanup cores
        _bind_manager.cleanup_bind_cores();
    }
}

// operator isolate
void V8Isolate::gc(bool full)
{
    _isolate->RequestGarbageCollectionForTesting(full ? ::v8::Isolate::kFullGarbageCollection : ::v8::Isolate::kMinorGarbageCollection);

    _isolate->LowMemoryNotification();
    _isolate->IdleNotificationDeadline(0);
}

// bind object
V8BindCoreObject* V8Isolate::translate_object(::skr::ScriptbleObject* obj)
{
    using namespace ::v8;
    Isolate::Scope isolate_scope(_isolate);
    return _bind_manager.translate_object(obj);
}
void V8Isolate::mark_object_deleted(::skr::ScriptbleObject* obj)
{
    using namespace ::v8;
    Isolate::Scope isolate_scope(_isolate);
    _bind_manager.mark_object_deleted(obj);
}

// bind value
V8BindCoreValue* V8Isolate::create_value(const RTTRType* type, const void* data)
{
    using namespace ::v8;
    Isolate::Scope isolate_scope(_isolate);
    return _bind_manager.create_value(type, data);
}
V8BindCoreValue* V8Isolate::translate_value_field(const RTTRType* type, const void* data, V8BindCoreRecordBase* owner)
{
    using namespace ::v8;
    Isolate::Scope isolate_scope(_isolate);
    return _bind_manager.translate_value_field(type, data, owner);
}

// make template
void V8Isolate::register_mapping_type(const RTTRType* type)
{
    using namespace ::v8;
    // v8 scope
    Isolate::Scope isolate_scope(_isolate);

    _bind_manager.register_mapping_type(type);
}
v8::Local<v8::ObjectTemplate> V8Isolate::_get_enum_template(const RTTRType* type)
{
    using namespace ::v8;
    // check
    SKR_ASSERT(type->is_enum());

    // v8 scope
    Isolate::Scope isolate_scope(_isolate);

    return _bind_manager.get_enum_template(type);
}
v8::Local<v8::FunctionTemplate> V8Isolate::_get_record_template(const RTTRType* type)
{
    using namespace ::v8;

    // check
    SKR_ASSERT(type->is_record());

    // v8 scope
    Isolate::Scope isolate_scope(_isolate);

    return _bind_manager.get_record_template(type);
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