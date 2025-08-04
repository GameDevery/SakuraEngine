#include <SkrV8/v8_context.hpp>
#include <SkrContainers/set.hpp>
#include <SkrCore/log.hpp>
#include <SkrRTTR/type.hpp>
#include <SkrV8/v8_bind.hpp>
#include <SkrV8/v8_isolate.hpp>

// v8 includes
#include <libplatform/libplatform.h>
#include <v8-initialization.h>
#include <v8-template.h>
#include <v8-external.h>
#include <v8-function.h>
#include <v8-container.h>

namespace skr
{
// ctor & dtor
V8Context::V8Context()
{
}
V8Context::~V8Context()
{
}

::v8::Global<::v8::Context> V8Context::v8_context() const
{
    return ::v8::Global<::v8::Context>(_isolate->v8_isolate(), _context);
}

// build export
void V8Context::build_export(FunctionRef<void(V8VirtualModule&)> build_func)
{
    auto               isolate = _isolate->v8_isolate();
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope    handle_scope(isolate);
    auto               context = _context.Get(isolate);
    v8::Context::Scope context_scope(context);

    // build module
    build_func(_virtual_module);

    // setup context
    _virtual_module.export_v8_to(
        isolate,
        context,
        _context.Get(isolate)->Global()
    );
}
bool V8Context::is_export_built() const
{
    return !_virtual_module.is_empty();
}
void V8Context::clear_export()
{
    if (is_export_built())
    {
        _virtual_module.erase_export(
            _isolate->v8_isolate(),
            _context.Get(_isolate->v8_isolate()),
            _context.Get(_isolate->v8_isolate())->Global()
        );
    }
}

// set & get global value
V8Value V8Context::get_global(StringView name)
{
    using namespace ::v8;

    // scopes
    auto           isolate = _isolate->v8_isolate();
    Isolate::Scope isolate_scope(isolate);
    HandleScope    handle_scope(isolate);
    Local<Context> context = _context.Get(isolate);
    Context::Scope context_scope(context);

    // find value
    auto maybe_value = context->Global()->Get(
        context,
        V8Bind::to_v8(name, true)
    );
    if (maybe_value.IsEmpty())
    {
        return {
            {},
            this
        };
    }
    else
    {
        return {
            { isolate, maybe_value.ToLocalChecked() },
            this
        };
    }
}

void V8Context::temp_run_script(StringView script)
{
    auto                   isolate = _isolate->v8_isolate();
    v8::Isolate::Scope     isolate_scope(isolate);
    v8::HandleScope        handle_scope(isolate);
    v8::Local<v8::Context> context = _context.Get(isolate);
    v8::Context::Scope     context_scope(context);

    // compile script
    v8::Local<v8::String> source = V8Bind::to_v8(script, false);
    v8::ScriptOrigin      origin(
        isolate,
        V8Bind::to_v8(u8"[CPP]"),
        0,
        0,
        true,
        -1,
        {},
        false,
        false,
        true,
        {}
    );
    auto may_be_script = ::v8::Script::Compile(
        context,
        source
    );

    // run script
    auto compiled_script = may_be_script.ToLocalChecked();
    auto exec_result     = compiled_script->Run(context);
}

// init & shutdown
void V8Context::_init(V8Isolate* isolate, String name)
{
    using namespace ::v8;

    _isolate = isolate;
    _name    = name;
    _virtual_module.set_isolate(_isolate);

    Isolate::Scope isolate_scope(_isolate->v8_isolate());
    HandleScope    handle_scope(_isolate->v8_isolate());

    // create context
    auto new_context = Context::New(_isolate->v8_isolate());
    _context.Reset(_isolate->v8_isolate(), new_context);

    // bind this
    new_context->SetAlignedPointerInEmbedderData(1, this);
}
void V8Context::_shutdown()
{
    // destroy context
    _context.Reset();
}
} // namespace skr