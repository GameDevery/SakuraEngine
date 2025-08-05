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
#include <v8-exception.h>

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
    _virtual_module.dump_bind_tp_error();

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
        return {};
    }
    else
    {
        return {
            { isolate, maybe_value.ToLocalChecked() },
            this
        };
    }
}

// exec
V8Value V8Context::exec(StringView script, bool as_module)
{
    using namespace ::v8;

    auto           isolate = _isolate->v8_isolate();
    Isolate::Scope isolate_scope(isolate);
    HandleScope    handle_scope(isolate);
    Local<Context> context = _context.Get(isolate);
    Context::Scope context_scope(context);

    if (as_module)
    {
        // compile module
        auto maybe_module = _compile_module(
            isolate,
            script,
            u8"[CPP]"
        );
        if (maybe_module.IsEmpty())
        {
            SKR_LOG_ERROR(u8"compile module failed");
            return {};
        }

        // run module
        return _exec_module(
            isolate,
            context,
            maybe_module.ToLocalChecked(),
            true
        );
    }
    else
    {
        // compile script
        auto maybe_script = _compile_script(
            isolate,
            context,
            script,
            u8"[CPP]"
        );
        if (maybe_script.IsEmpty())
        {
            SKR_LOG_ERROR(u8"compile script failed");
            return {};
        }

        // run script
        return _exec_script(
            isolate,
            context,
            maybe_script.ToLocalChecked(),
            true
        );
    }
}
V8Value V8Context::exec_file(StringView file_path, bool as_module)
{
    using namespace ::v8;

    auto           isolate = _isolate->v8_isolate();
    Isolate::Scope isolate_scope(isolate);
    HandleScope    handle_scope(isolate);
    Local<Context> context = _context.Get(isolate);
    Context::Scope context_scope(context);

    SKR_UNIMPLEMENTED_FUNCTION();

    return {};
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

// exec helpers
v8::MaybeLocal<v8::Script> V8Context::_compile_script(
    v8::Isolate*           isolate,
    v8::Local<v8::Context> context,
    StringView             script,
    StringView             path
)
{
    v8::Local<v8::String> source = V8Bind::to_v8(script, false);
    v8::ScriptOrigin      origin(
        isolate,
        V8Bind::to_v8(path),
        0,
        0,
        true,
        -1,
        {},
        false,
        false,
        false,
        {}
    );
    return ::v8::Script::Compile(
        context,
        source
    );
}
v8::MaybeLocal<v8::Module> V8Context::_compile_module(
    v8::Isolate* isolate,
    StringView   script,
    StringView   path
)
{
    v8::ScriptOrigin origin(
        isolate,
        V8Bind::to_v8(path),
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
    ::v8::ScriptCompiler::Source source(
        V8Bind::to_v8(script),
        origin
    );
    return ::v8::ScriptCompiler::CompileModule(
        isolate,
        &source,
        v8::ScriptCompiler::kNoCompileOptions
    );
}
V8Value V8Context::_exec_script(
    v8::Isolate*           isolate,
    v8::Local<v8::Context> context,
    v8::Local<v8::Script>  script,
    bool                   dump_exception
)
{
    using namespace ::v8;
    TryCatch try_catch(isolate);

    // run script
    auto exec_result = script->Run(context);

    // dump exception
    if (try_catch.HasCaught())
    {
        if (dump_exception)
        {
            String exception_str;
            V8Bind::to_native(try_catch.Exception()->ToString(context).ToLocalChecked(), exception_str);
            SKR_LOG_FMT_ERROR(
                u8"[V8] uncaught exception: {}",
                exception_str.c_str()
            );
        }
        try_catch.ReThrow();
    }

    // return result
    if (exec_result.IsEmpty())
    {
        return {};
    }
    else
    {
        return {
            { isolate, exec_result.ToLocalChecked() },
            this
        };
    }
}
V8Value V8Context::_exec_module(
    v8::Isolate*           isolate,
    v8::Local<v8::Context> context,
    v8::Local<v8::Module>  module,
    bool                   dump_exception
)
{
    using namespace ::v8;
    TryCatch try_catch(isolate);

    // instantiate module
    auto instantiate_result = module->InstantiateModule(context, _resolve_module);
    if (instantiate_result.IsNothing())
    {
        SKR_LOG_ERROR(u8"instantiate module failed");
        return {};
    }

    // evaluate module
    auto eval_result = module->Evaluate(context);

    // check module error
    if (module->GetStatus() == v8::Module::kErrored)
    {
        // won't stop on first level exception, see https://github.com/nodejs/node/issues/50430
        _isolate->v8_isolate()->ThrowException(module->GetException());
    }

    // finish first level promise
    if (eval_result.ToLocalChecked()->IsPromise())
    {
        auto promise = eval_result.ToLocalChecked().As<v8::Promise>();
        while (promise->State() == v8::Promise::kPending)
        {
            _isolate->v8_isolate()->PerformMicrotaskCheckpoint();
        }
        if (promise->State() == v8::Promise::kRejected)
        {
            // promise->MarkAsHandled();
            _isolate->v8_isolate()->ThrowException(promise->Result());
        }
    }

    // dump exception
    if (try_catch.HasCaught())
    {
        if (dump_exception)
        {
            String exception_str;
            V8Bind::to_native(try_catch.Exception()->ToString(context).ToLocalChecked(), exception_str);
            SKR_LOG_FMT_ERROR(
                u8"[V8] uncaught exception: {}",
                exception_str.c_str()
            );
        }
        try_catch.ReThrow();
    }

    // return result
    if (eval_result.IsEmpty())
    {
        return {};
    }
    else
    {
        return {
            { isolate, eval_result.ToLocalChecked() },
            this
        };
    }
}

// callback
v8::MaybeLocal<v8::Module> V8Context::_resolve_module(
    v8::Local<v8::Context>    context,
    v8::Local<v8::String>     specifier,
    v8::Local<v8::FixedArray> import_assertions,
    v8::Local<v8::Module>     referrer
)
{
    auto isolate     = v8::Isolate::GetCurrent();
    auto skr_isolate = reinterpret_cast<V8Isolate*>(isolate->GetData(0));

    // get module name
    skr::String module_name;
    if (!V8Bind::to_native(specifier, module_name))
    {
        isolate->ThrowException(V8Bind::to_v8(u8"failed to convert module name"));
        SKR_LOG_ERROR(u8"failed to convert module name");
        return {};
    }

    // solve module kind
    bool is_cpp_module = true;

    // TODO. solve module
    // solve module
    // if (is_cpp_module)
    // { // cpp module
    //     // find module
    //     auto found_module = skr_isolate->find_cpp_module(module_name);
    //     if (!found_module)
    //     {
    //         isolate->ThrowException(V8Bind::to_v8(u8"cannot find module"));
    //         SKR_LOG_FMT_ERROR(u8"module {} not found", module_name);
    //         return {};
    //     }

    //     // return module
    //     return found_module->v8_module();
    // }
    // else
    // {
    //     return {};
    // }

    return {};
}
} // namespace skr