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
    context->exec(u8"SimpleTest.print('fuck')");
    context->exec(u8R"__(
        print_sum = function (x, y) {
            SimpleTest.print(`${x} + ${y} = ${x + y}`)
        }
        obj = {
            x: 1,
            y: 2,
            z: 3,
            print: function (prefix) {
                SimpleTest.print(`${prefix}: {x, y, z}`)
            }
        }
    )__");
    auto print_sum = context->get_global(u8"print_sum");
    auto obj       = context->get_global(u8"obj");

    print_sum.call<void, int32_t, int32_t>(1, 1);
    obj.call_method<void, skr::StringView>(u8"print", u8"print self");

    SKR_LOG_FMT_INFO(u8"Test End");
}