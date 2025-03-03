#include "SkrV8/v8_context.hpp"
#include "SkrCore/log.hpp"
#include "SkrV8/v8_isolate.hpp"

namespace skr::v8
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

// getter
::v8::Global<::v8::Context> V8Context::v8_context() const
{
    return ::v8::Global<::v8::Context>(_isolate->v8_isolate(), _context);
}

// exec script
void V8Context::exec_script(StringView script)
{
    ::v8::Isolate::Scope       isolate_scope(_isolate->v8_isolate());
    ::v8::HandleScope          handle_scope(_isolate->v8_isolate());
    ::v8::Local<::v8::Context> solved_context = _context.Get(_isolate->v8_isolate());
    ::v8::Context::Scope       context_scope(solved_context);

    // compile script
    ::v8::Local<::v8::String> source = ::v8::String::NewFromUtf8(
                                       _isolate->v8_isolate(),
                                       reinterpret_cast<const char*>(script.data_raw()),
                                       ::v8::NewStringType::kNormal,
                                       script.size())
                                       .ToLocalChecked();
    auto compiled_script = ::v8::Script::Compile(solved_context, source);
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

} // namespace skr::v8