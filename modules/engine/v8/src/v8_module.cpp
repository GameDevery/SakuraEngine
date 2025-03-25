#include <SkrV8/v8_module.hpp>
#include <SkrV8/v8_isolate.hpp>

namespace skr
{
// ctor & dtor
V8Module::V8Module(V8Isolate* isolate)
    : _isolate(isolate)
{
}
V8Module::~V8Module()
{
}

// init & shutdown
void V8Module::finalize()
{
}
void V8Module::shutdown()
{
}
} // namespace skr