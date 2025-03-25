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
void V8Context::register_type(skr::RTTRType* type, StringView name_space)
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
        auto ctx = _context.Get(_isolate->v8_isolate());
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
        if (flag_all(type->record_flag(), ERTTRRecordFlag::ScriptMapping))
        { // mapping mode
            _isolate->register_mapping_type(type);
        }
        else
        { // value or object
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
}

// getter
::v8::Global<::v8::Context> V8Context::v8_context() const
{
    return ::v8::Global<::v8::Context>(_isolate->v8_isolate(), _context);
}

// run as script
V8Value V8Context::exec_script(StringView script)
{
    auto                   isolate = _isolate->v8_isolate();
    v8::Isolate::Scope     isolate_scope(isolate);
    v8::HandleScope        handle_scope(isolate);
    v8::Local<v8::Context> context = _context.Get(isolate);
    v8::Context::Scope     context_scope(context);

    // compile script
    v8::Local<v8::String> source        = V8Bind::to_v8(script, false);
    auto                  may_be_script = ::v8::Script::Compile(context, source);
    if (may_be_script.IsEmpty())
    {
        SKR_LOG_ERROR(u8"compile script failed");
        return {};
    }

    // run script
    auto compiled_script = may_be_script.ToLocalChecked();
    auto exec_result     = compiled_script->Run(context);
    if (!exec_result.IsEmpty())
    {
        V8Value result;
        result.context = this;
        result.v8_value.Reset(_isolate->v8_isolate(), exec_result.ToLocalChecked());
        return result;
    }

    return {};
}

// run as ES module
V8Value V8Context::exec_module(StringView script)
{
    auto                   isolate = _isolate->v8_isolate();
    v8::Isolate::Scope     isolate_scope(isolate);
    v8::HandleScope        handle_scope(isolate);
    v8::Local<v8::Context> context = _context.Get(isolate);
    v8::Context::Scope     context_scope(context);

    // compile module
    v8::ScriptOrigin origin(
        isolate,
        V8Bind::to_v8(skr::StringView{ u8"" }),
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
    auto                         source_str = V8Bind::to_v8(script, false);
    ::v8::ScriptCompiler::Source source(source_str, origin);
    auto                         maybe_module = ::v8::ScriptCompiler::CompileModule(
        isolate,
        &source,
        v8::ScriptCompiler::kNoCompileOptions
    );
    if (maybe_module.IsEmpty())
    {
        SKR_LOG_ERROR(u8"compile module failed");
        return {};
    }

    // instantiate module
    auto module             = maybe_module.ToLocalChecked();
    auto instantiate_result = module->InstantiateModule(context, _resolve_module);
    if (instantiate_result.IsNothing())
    {
        SKR_LOG_ERROR(u8"instantiate module failed");
        return {};
    }

    // evaluate module
    auto eval_result = module->Evaluate(context);
    if (!eval_result.IsEmpty())
    {
        V8Value result;
        result.context = this;
        result.v8_value.Reset(_isolate->v8_isolate(), eval_result.ToLocalChecked());
        return result;
    }

    return {};
}

// callback
v8::MaybeLocal<v8::Module> V8Context::_resolve_module(
    v8::Local<v8::Context>    context,
    v8::Local<v8::String>     specifier,
    v8::Local<v8::FixedArray> import_assertions,
    v8::Local<v8::Module>     referrer
)
{
    auto isolate = v8::Isolate::GetCurrent();

    skr::String spec;
    V8Bind::to_native(specifier, spec);

    SKR_LOG_FMT_INFO(u8"resolve module: {}", spec.c_str());

    std::vector<v8::Local<v8::String>> imports = {
        V8Bind::to_v8(u8"name", false)
    };
    auto callback = +[](v8::Local<v8::Context> context, v8::Local<v8::Module> module) -> v8::MaybeLocal<v8::Value> {
        auto isolate = v8::Isolate::GetCurrent();

        // clang-format off
        module->SetSyntheticModuleExport(
            isolate,
            V8Bind::to_v8(u8"name", false),
            V8Bind::to_v8(u8"圆头", false)
        ).Check();
        // clang-format on

        return v8::MaybeLocal<v8::Value>(v8::True(isolate));
    };

    v8::Local<v8::Module> module = v8::Module::CreateSyntheticModule(
        isolate,
        specifier,
        imports,
        callback
    );

    return module;
}

} // namespace skr