#pragma once
#include "SkrBase/config.h"
#include "SkrContainers/string.hpp"
#include "SkrRTTR/scriptble_object.hpp"
#include "v8-context.h"
#include "v8-isolate.h"
#include "v8-persistent-handle.h"
#include "v8_isolate.hpp"
#include <SkrCore/log.hpp>
#include "SkrV8/v8_bind.hpp"
#include "SkrV8/v8_isolate.hpp"

namespace skr
{
struct V8Isolate;
struct V8Context;

struct V8Value {
    bool is_empty() const;

    template <typename T>
    Optional<T> get() const;

    void reset();

    v8::Global<v8::Value> v8_value = {};
    V8Context*            context  = nullptr;
};

struct SKR_V8_API V8Context {
    // ctor & dtor
    V8Context(V8Isolate* isolate);
    ~V8Context();

    // delete copy & move
    V8Context(const V8Context&)            = delete;
    V8Context(V8Context&&)                 = delete;
    V8Context& operator=(const V8Context&) = delete;
    V8Context& operator=(V8Context&&)      = delete;

    // init & shutdown
    void init();
    void shutdown();

    // register type
    void register_type(skr::RTTRType* type);
    template <typename T>
    void register_type();

    // getter
    ::v8::Global<::v8::Context> v8_context() const;
    inline V8Isolate*           isolate() const { return _isolate; }

    // set global value
    template <typename T>
    void set_global(StringView name, T&& v);

    // run script
    V8Value exec_script(StringView script);

private:
    // owner
    V8Isolate* _isolate;

    // context data
    ::v8::Persistent<::v8::Context> _context;
};
} // namespace skr

namespace skr
{
template <typename T>
inline void V8Context::register_type()
{
    if (auto type = skr::type_of<T>())
    {
        register_type(type);
    }
    else
    {
        SKR_LOG_FMT_ERROR(u8"failed to register type {}", skr::type_name_of<T>());
        return;
    }
}

inline bool V8Value::is_empty() const
{
    return v8_value.IsEmpty();
}
template <typename T>
inline Optional<T> V8Value::get() const
{
    using namespace ::v8;

    // scopes
    auto*          isolate = context->isolate()->v8_isolate();
    Isolate::Scope isolate_scope(isolate);
    HandleScope    handle_scope(isolate);

    // solve context
    Local<Context> solved_context = context->v8_context().Get(isolate);
    Context::Scope context_scope(solved_context);
    auto*          bind_manager    = &context->isolate()->_bind_manager;
    Local<Value>   solved_v8_value = v8_value.Get(isolate);

    if constexpr (
        std::is_integral_v<T> ||
        std::is_floating_point_v<T> ||
        std::is_same_v<T, bool> ||
        std::is_same_v<T, skr::String> ||
        std::is_same_v<T, skr::StringView>
    )
    {
        T result;
        if (V8Bind::to_native(solved_v8_value, result))
        {
            return { result };
        }
        else
        {
            return {};
        }
    }
    else if constexpr (std::is_pointer_v<T>)
    {
        using RawType = std::remove_pointer_t<T>;

        if constexpr (std::derived_from<RawType, ScriptbleObject>)
        {
            T result;
            if (bind_manager->to_native(type_of<RawType>(), &result, solved_v8_value, false))
            {
                return { result };
            }
            else
            {
                return {};
            }
        }
        else
        {
            return {};
        }
    }
    else
    {
        T result;
        if (bind_manager->to_native(type_of<T>(), &result, solved_v8_value, true))
        {
            return { result };
        }
        else
        {
            return {};
        }
    }
}
inline void V8Value::reset()
{
    v8_value.Reset();
    context = nullptr;
}

template <typename T>
inline void V8Context::set_global(StringView name, T&& v)
{
    using DecayType = std::decay_t<T>;

    using namespace ::v8;

    // scopes
    Isolate::Scope isolate_scope(_isolate->v8_isolate());
    HandleScope    handle_scope(_isolate->v8_isolate());

    // solve context
    Local<Context> solved_context = _context.Get(_isolate->v8_isolate());
    Context::Scope context_scope(solved_context);
    Local<Object>  global       = solved_context->Global();
    auto*          bind_manager = &_isolate->_bind_manager;

    // translate value
    Local<Value> value;
    if constexpr (
        std::is_integral_v<DecayType> ||
        std::is_floating_point_v<DecayType> ||
        std::is_same_v<DecayType, bool> ||
        std::is_same_v<DecayType, skr::String> ||
        std::is_same_v<DecayType, skr::StringView>
    )
    {
        value = V8Bind::to_v8(v);
    }
    else if constexpr (std::is_pointer_v<DecayType>)
    {
        using RawType = std::remove_pointer_t<DecayType>;

        if constexpr (std::derived_from<RawType, ScriptbleObject>)
        {
            value = bind_manager->translate_object(v)->v8_object.Get(_isolate->v8_isolate());
        }
        else
        {
            value = bind_manager->create_value(type_of<RawType>(), v)->v8_object.Get(_isolate->v8_isolate());
        }
    }
    else
    {
        value = bind_manager->create_value(type_of<DecayType>(), &v)->v8_object.Get(_isolate->v8_isolate());
    }

    // set value
    // clang-format off
    global->Set(
        solved_context,
        V8Bind::to_v8(name, true),
        value
    ).Check();
    // clang-format on
}
} // namespace skr