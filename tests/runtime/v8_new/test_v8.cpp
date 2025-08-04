#include "SkrTestFramework/framework.hpp"
#include "SkrRTTR/rttr_traits.hpp"
#include "SkrCore/log.hpp"
#include "test_v8_types.hpp"
#include "SkrV8/v8_isolate.hpp"
#include "SkrV8/v8_context.hpp"

skr::V8Isolate isolate;

struct V8EnvGuard {
    V8EnvGuard()
    {
        skr::init_v8();
        isolate.init();
    }
    ~V8EnvGuard()
    {
        isolate.shutdown();
        skr::shutdown_v8();
    }
} _guard{};

TEST_CASE("simple")
{
    auto* context = isolate.main_context();
    SKR_DEFER({ isolate.destroy_context(context); });

    SKR_LOG_FMT_INFO(u8"Test Begin");

    context->build_export([](skr::V8VirtualModule& module) {
        module.register_type<test_v8::SimpleTest>(u8"");
    });
    context->temp_run_script(u8"SimpleTest.print()");
    SKR_LOG_FMT_INFO(u8"Test End");
}