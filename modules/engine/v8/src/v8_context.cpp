#include "SkrV8/v8_context.hpp"
#include "SkrCore/log.hpp"
#include "SkrV8/v8_isolate.hpp"
#include "SkrV8/v8_bind.hpp"
#include "v8-function.h"

namespace skr
{
// ctor & dtor
V8Context::V8Context(V8Isolate* isolate)
    : _isolate(isolate)
{
}
V8Context::~V8Context()
{
}

// init & shutdown
void V8Context::init()
{
    using namespace ::v8;
    Isolate::Scope isolate_scope(_isolate->v8_isolate());
    HandleScope    handle_scope(_isolate->v8_isolate());

    // create context
    auto new_context = Context::New(_isolate->v8_isolate());
    _context.Reset(_isolate->v8_isolate(), new_context);

    // bind this
    new_context->SetAlignedPointerInEmbedderData(1, this);
}
void V8Context::shutdown()
{
    // destroy context
    _context.Reset();
}

// register type
void V8Context::register_type(skr::RTTRType* type)
{
    v8::Isolate::Scope isolate_scope(_isolate->v8_isolate());
    v8::HandleScope    handle_scope(_isolate->v8_isolate());
    v8::Context::Scope context_scope(_context.Get(_isolate->v8_isolate()));

    if (type->is_enum())
    {
        // get template
        auto template_ref = _isolate->_get_enum_template(type);
        if (template_ref.IsEmpty())
        {
            SKR_LOG_FMT_ERROR(u8"failed to get template for type {}", type->name());
            return;
        }

        // inject to self
        auto ctx  = _context.Get(_isolate->v8_isolate());
        auto obj = template_ref->NewInstance(ctx).ToLocalChecked();
        // clang-format off
        auto set_result = ctx->Global()->Set(
            ctx,
            V8Bind::to_v8(type->name(), true),
            obj
        );
        // clang-format on

        // check set result
        if (set_result.IsNothing())
        {
            SKR_LOG_FMT_ERROR(u8"failed to set template for type {}", type->name());
            return;
        }

    }
    else
    {
        // get template
        auto template_ref = _isolate->_get_record_template(type);
        if (template_ref.IsEmpty())
        {
            SKR_LOG_FMT_ERROR(u8"failed to get template for type {}", type->name());
            return;
        }
    
        // inject to self
        auto ctx  = _context.Get(_isolate->v8_isolate());
        auto func = template_ref->GetFunction(ctx).ToLocalChecked();
        // clang-format off
        auto set_result = ctx->Global()->Set(
            ctx,
            V8Bind::to_v8(type->name(), true),
            func
        );
        // clang-format on
    
        // check set result
        if (set_result.IsNothing())
        {
            SKR_LOG_FMT_ERROR(u8"failed to set template for type {}", type->name());
            return;
        }
    }

}

// getter
::v8::Global<::v8::Context> V8Context::v8_context() const
{
    return ::v8::Global<::v8::Context>(_isolate->v8_isolate(), _context);
}

// set global value
void V8Context::set_global(StringView name, uint32_t v)
{
    using namespace ::v8;

    // scopes
    Isolate::Scope isolate_scope(_isolate->v8_isolate());
    HandleScope    handle_scope(_isolate->v8_isolate());

    // solve context
    Local<Context> solved_context = _context.Get(_isolate->v8_isolate());
    Context::Scope context_scope(solved_context);
    Local<Object>  global = solved_context->Global();

    // translate value
    Local<::v8::String> key   = V8Bind::to_v8(name, true);
    Local<Value>        value = Integer::New(_isolate->v8_isolate(), v);

    // set
    global->Set(solved_context, key, value).Check();
}
void V8Context::set_global(StringView name, int32_t v)
{
    using namespace ::v8;

    // scopes
    Isolate::Scope isolate_scope(_isolate->v8_isolate());
    HandleScope    handle_scope(_isolate->v8_isolate());

    // solve context
    Local<Context> solved_context = _context.Get(_isolate->v8_isolate());
    Context::Scope context_scope(solved_context);
    Local<Object>  global = solved_context->Global();

    // translate value
    Local<::v8::String> key   = V8Bind::to_v8(name, true);
    Local<Value>        value = Integer::New(_isolate->v8_isolate(), v);

    // set
    global->Set(solved_context, key, value).Check();
}
void V8Context::set_global(StringView name, uint64_t v)
{
    using namespace ::v8;

    // scopes
    Isolate::Scope isolate_scope(_isolate->v8_isolate());
    HandleScope    handle_scope(_isolate->v8_isolate());

    // solve context
    Local<Context> solved_context = _context.Get(_isolate->v8_isolate());
    Context::Scope context_scope(solved_context);
    Local<Object>  global = solved_context->Global();

    // translate value
    Local<::v8::String> key   = V8Bind::to_v8(name, true);
    Local<Value>        value = BigInt::New(_isolate->v8_isolate(), v);

    // set
    global->Set(solved_context, key, value).Check();
}
void V8Context::set_global(StringView name, int64_t v)
{
    using namespace ::v8;

    // scopes
    Isolate::Scope isolate_scope(_isolate->v8_isolate());
    HandleScope    handle_scope(_isolate->v8_isolate());

    // solve context
    Local<Context> solved_context = _context.Get(_isolate->v8_isolate());
    Context::Scope context_scope(solved_context);
    Local<Object>  global = solved_context->Global();

    // translate value
    Local<::v8::String> key   = V8Bind::to_v8(name, true);
    Local<Value>        value = BigInt::New(_isolate->v8_isolate(), v);

    // set
    global->Set(solved_context, key, value).Check();
}
void V8Context::set_global(StringView name, double v)
{
    using namespace ::v8;

    // scopes
    Isolate::Scope isolate_scope(_isolate->v8_isolate());
    HandleScope    handle_scope(_isolate->v8_isolate());

    // solve context
    Local<Context> solved_context = _context.Get(_isolate->v8_isolate());
    Context::Scope context_scope(solved_context);
    Local<Object>  global = solved_context->Global();

    // translate value
    Local<::v8::String> key   = V8Bind::to_v8(name, true);
    Local<Value>        value = Number::New(_isolate->v8_isolate(), v);

    // set
    global->Set(solved_context, key, value).Check();
}
void V8Context::set_global(StringView name, bool v)
{
    using namespace ::v8;

    // scopes
    Isolate::Scope isolate_scope(_isolate->v8_isolate());
    HandleScope    handle_scope(_isolate->v8_isolate());

    // solve context
    Local<Context> solved_context = _context.Get(_isolate->v8_isolate());
    Context::Scope context_scope(solved_context);
    Local<Object>  global = solved_context->Global();

    // translate value
    Local<::v8::String> key   = V8Bind::to_v8(name, true);
    Local<Value>        value = Boolean::New(_isolate->v8_isolate(), v);

    // set
    global->Set(solved_context, key, value).Check();
}
void V8Context::set_global(StringView name, StringView v)
{
    using namespace ::v8;

    // scopes
    Isolate::Scope isolate_scope(_isolate->v8_isolate());
    HandleScope    handle_scope(_isolate->v8_isolate());

    // solve context
    Local<Context> solved_context = _context.Get(_isolate->v8_isolate());
    Context::Scope context_scope(solved_context);
    Local<Object>  global = solved_context->Global();

    // translate value
    Local<::v8::String> key   = V8Bind::to_v8(name, true);
    Local<Value>        value = V8Bind::to_v8(v, false);

    // set
    global->Set(solved_context, key, value).Check();
}
void V8Context::set_global(StringView name, skr::ScriptbleObject* obj)
{
    using namespace ::v8;

    // scopes
    Isolate::Scope isolate_scope(_isolate->v8_isolate());
    HandleScope    handle_scope(_isolate->v8_isolate());

    // solve context
    Local<Context> solved_context = _context.Get(_isolate->v8_isolate());
    Context::Scope context_scope(solved_context);
    Local<Object>  global = solved_context->Global();

    // translate value
    Local<::v8::String> key   = V8Bind::to_v8(name, true);
    Local<Object>       value = _isolate->translate_object(obj)->v8_object.Get(_isolate->v8_isolate());

    // set
    global->Set(solved_context, key, value).Check();
}

// exec script
void V8Context::exec_script(StringView script)
{
    ::v8::Isolate::Scope       isolate_scope(_isolate->v8_isolate());
    ::v8::HandleScope          handle_scope(_isolate->v8_isolate());
    ::v8::Local<::v8::Context> solved_context = _context.Get(_isolate->v8_isolate());
    ::v8::Context::Scope       context_scope(solved_context);

    // compile script
    ::v8::Local<::v8::String> source          = V8Bind::to_v8(script, false);
    auto                      compiled_script = ::v8::Script::Compile(solved_context, source);
    if (compiled_script.IsEmpty())
    {
        SKR_LOG_ERROR(u8"compile script failed");
        return;
    }

    // run script
    auto result = compiled_script.ToLocalChecked()->Run(solved_context);
    if (!result.IsEmpty())
    {
        ::v8::String::Utf8Value utf8(_isolate->v8_isolate(), result.ToLocalChecked());
        SKR_LOG_WARN(u8"exec script result: %s", *utf8);
    }
}

} // namespace skr