#pragma once
#include <SkrCore/memory/rc.hpp>
#include <SkrRTTR/script/script_binder.hpp>
#include <SkrRTTR/script/stack_proxy.hpp>
#include <SkrV8/bind_template/v8_bind_template.hpp>

// v8 includes
#include <v8-isolate.h>
#include <v8-platform.h>
#include <v8-primitive.h>
#ifndef __meta__
    #include "SkrV8New/v8_env.generated.h"
#endif

// isolate
namespace skr
{
struct V8Context;
struct V8Isolate;

// clang-format off
sreflect_struct(
    guid = "921187d2-4d38-42b5-81b1-cc79d5739cef"
    rttr.exclude_bases += "skr::IV8BindManager"
)
SKR_V8_API V8Isolate : IScriptMixinCore, IV8BindManager {
    // clang-format on
    SKR_GENERATE_BODY()

    // ctor & dtor
    V8Isolate();
    ~V8Isolate();

    // delate copy & move
    V8Isolate(const V8Isolate&)            = delete;
    V8Isolate(V8Isolate&&)                 = delete;
    V8Isolate& operator=(const V8Isolate&) = delete;
    V8Isolate& operator=(V8Isolate&&)      = delete;

    // init & shutdown
    void init();
    void shutdown();
    bool is_init() const;

    // isolate operators
    void pump_message_loop();
    void gc(bool full = true);

    // context management
    V8Context* main_context() const;
    V8Context* create_context(String name = {});
    void       destroy_context(V8Context* context);

    // getter
    inline v8::Isolate* v8_isolate() const { return _isolate; }

    //==> IScriptMixinCore API
    void on_object_destroyed(
        ScriptbleObject* obj
    ) override;
    bool try_invoke_mixin(
        ScriptbleObject*             obj,
        StringView                   name,
        const span<const StackProxy> args,
        StackProxy                   result
    ) override;
    //==> IScriptMixinCore API

    //==> IV8BindManager API
    // bind proxy management
    V8BindTemplate* solve_bind_tp(
        TypeSignatureView signature
    ) override;
    V8BindTemplate* solve_bind_tp(
        const GUID& type_id
    ) override;

    // bind proxy management
    void add_bind_proxy(
        void*        native_ptr,
        V8BindProxy* bind_proxy
    ) override;
    void remove_bind_proxy(
        void*        native_ptr,
        V8BindProxy* bind_proxy
    ) override;
    V8BindProxy* find_bind_proxy(
        void* native_ptr
    ) const override;

    // 用于缓存调用期间为参数创建的临时 bind proxy
    void push_param_scope() override;
    void pop_param_scope() override;
    void push_param_proxy(
        V8BindProxy* bind_proxy
    ) override;

    // mixin
    IScriptMixinCore* get_mixin_core() const override;

    // get logger
    ErrorCollector& logger() override { return _logger; }
    //==> IV8BindManager API

private:
    // isolate data
    v8::Isolate*              _isolate               = nullptr;
    v8::Isolate::CreateParams _isolate_create_params = {};

    // bind tp cache
    Map<GUID, V8BindTemplate*>          _bind_tp_map         = {};
    Map<TypeSignature, V8BindTemplate*> _bind_tp_map_generic = {};

    // bind proxy cache
    Map<void*, V8BindProxy*> _bind_proxy_map = {};

    // context manage
    V8Context*              _main_context = nullptr;
    Map<String, V8Context*> _contexts     = {};

    ErrorCollector _logger = {};

    // TODO. cpp module manage
    // TODO. script module manage
    // TODO. debugger
};

struct V8Value {
    // ops
    bool is_empty() const;
    void reset();

    // get value & invoke
    template <typename T>
    Optional<T> as() const;
    template <typename T>
    Optional<T> is() const;
    V8Value     get(StringView name) const;
    template <typename Ret, typename... Args>
    decltype(auto) call(Args&&... args) const;

    // content
    v8::Global<v8::Value> v8_value = {};
    V8Context*            context  = nullptr;
};

struct SKR_V8_API V8Context {
    SKR_RC_IMPL();

    friend struct V8Isolate;

    // ctor & dtor
    V8Context();
    ~V8Context();

    // delete copy & move
    V8Context(const V8Context&)            = delete;
    V8Context(V8Context&&)                 = delete;
    V8Context& operator=(const V8Context&) = delete;
    V8Context& operator=(V8Context&&)      = delete;

    // getter
    ::v8::Global<::v8::Context> v8_context() const;
    inline V8Isolate*           isolate() const { return _isolate; }
    inline const String&        name() const { return _name; }

    // temp api
    void temp_register(const GUID& type_id);
    template <typename T>
    inline void temp_register()
    {
        temp_register(type_id_of<T>());
    }
    void temp_run_script(StringView script);

private:
    // init & shutdown
    void _init(V8Isolate* isolate, String name);
    void _shutdown();

private:
    V8Isolate* _isolate = nullptr;

    v8::Persistent<v8::Context> _context = {};
    String                      _name    = {};
};

} // namespace skr

// global init
namespace skr
{
SKR_V8_API void init_v8();
SKR_V8_API void shutdown_v8();
SKR_V8_API v8::Platform& get_v8_platform();
} // namespace skr

// V8Value impl
namespace skr
{
// ops
inline bool V8Value::is_empty() const
{
    return v8_value.IsEmpty();
}
inline void V8Value::reset()
{
    v8_value.Reset();
    context = nullptr;
}

// get value & invoke
template <typename T>
inline Optional<T> V8Value::as() const
{
    SKR_ASSERT(is<T>());
}
template <typename T>
inline Optional<T> V8Value::is() const
{
}
inline V8Value V8Value::get(StringView name) const
{
    return {};
}
template <typename Ret, typename... Args>
inline decltype(auto) V8Value::call(Args&&... args) const
{
    using namespace ::v8;
}
} // namespace skr