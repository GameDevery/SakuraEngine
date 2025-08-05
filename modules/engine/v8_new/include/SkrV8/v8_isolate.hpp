#pragma once
#include <SkrCore/memory/rc.hpp>
#include <SkrRTTR/script/scriptble_object.hpp>
#include <SkrRTTR/script/stack_proxy.hpp>
#include <SkrCore/error_collector.hpp>
#include <SkrV8/v8_fwd.hpp>
#include <SkrV8/bind_template/v8_bind_template.hpp>

// v8 includes
#include <v8-isolate.h>
#include <v8-platform.h>
#include <v8-primitive.h>
#ifndef __meta__
    #include "SkrV8/v8_isolate.generated.h"
#endif

namespace skr
{
// clang-format off
sreflect_struct(
    guid = "921187d2-4d38-42b5-81b1-cc79d5739cef"
)
SKR_V8_NEW_API V8Isolate : IScriptMixinCore {
    // clang-format on
    SKR_GENERATE_BODY(V8Isolate)

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

    // invoke helper
    bool invoke_v8(
        v8::Local<v8::Value>    v8_this,
        v8::Local<v8::Function> v8_func,
        span<const StackProxy>  params,
        StackProxy              return_value
    );

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

    // bind proxy management
    V8BindTemplate* solve_bind_tp(
        TypeSignatureView signature
    );
    V8BindTemplate* solve_bind_tp(
        const GUID& type_id
    );
    template <typename T>
    inline T* solve_bind_tp_as(const GUID& type_id)
    {
        return solve_bind_tp(type_id)->as<T>();
    }

    template <typename T>
    inline T* solve_bind_tp_as(TypeSignatureView type_id)
    {
        return solve_bind_tp(type_id)->as<T>();
    }

    // bind proxy management
    void add_bind_proxy(
        void*        native_ptr,
        V8BindProxy* bind_proxy
    );
    void remove_bind_proxy(
        void*        native_ptr,
        V8BindProxy* bind_proxy
    );
    V8BindProxy* find_bind_proxy(
        void* native_ptr
    ) const;

    // 用于缓存调用期间为参数创建的临时 bind proxy
    void push_param_scope();
    void pop_param_scope();
    void push_param_proxy(
        V8BindProxy* bind_proxy
    );

    // mixin
    IScriptMixinCore* get_mixin_core() const;

private:
    // helper

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

    // call v8 bind proxy manage
    Vector<V8BindProxy*> _call_v8_param_proxy       = {};
    Vector<uint64_t>     _call_v8_param_proxy_stack = {};

    // TODO. cpp module manage
    // TODO. script module manage
    // TODO. debugger
};
} // namespace skr

// global init
namespace skr
{
SKR_V8_NEW_API void init_v8();
SKR_V8_NEW_API void shutdown_v8();
SKR_V8_NEW_API v8::Platform& get_v8_platform();
} // namespace skr