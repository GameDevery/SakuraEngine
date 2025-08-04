#pragma once
#include <SkrCore/memory/rc.hpp>
#include <SkrRTTR/script/script_binder.hpp>
#include <SkrRTTR/script/stack_proxy.hpp>
#include <SkrV8/bind_template/v8_bind_template.hpp>
#include <SkrV8/v8_bind.hpp>
#include <SkrV8/v8_virtual_module.hpp>

// v8 includes
#include <v8-isolate.h>
#include <v8-platform.h>
#include <v8-primitive.h>

namespace skr
{
struct V8Context;
struct V8Isolate;

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

    // build export
    void build_export(FunctionRef<void(V8VirtualModule&)> build_func);
    bool is_export_built() const;
    void clear_export();

    // temp api
    void temp_run_script(StringView script);

private:
    // init & shutdown
    void _init(V8Isolate* isolate, String name);
    void _shutdown();

private:
    V8Isolate*                  _isolate        = nullptr;
    v8::Persistent<v8::Context> _context        = {};
    String                      _name           = {};
    V8VirtualModule             _virtual_module = {};
};
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