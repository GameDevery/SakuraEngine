#include "SkrV8/v8_isolate.hpp"
#include "SkrV8/v8_context.hpp"
#include "SkrRTTR/export/export_builder.hpp"
#include "SkrRTTR/rttr_traits.hpp"
#include "SkrRTTR/type.hpp"
#include "SkrCore/log.hpp"
#include "test_types.hpp"

int32_t test_v8::TestType::static_value = 0;

int main(int argc, char* argv[])
{
    using namespace skr::v8;

    V8Isolate isolate;
    V8Context context(&isolate);

    // init
    init_v8();
    isolate.init();
    context.init();

    // import types
    isolate.make_record_template(skr::rttr::type_of<test_v8::TestType>());

    // inject into context
    isolate.inject_templates_into_context(context.v8_context());

    // setup global
    context.set_global(u8"g_add_value", 100);

    // exec code
    context.exec_script(u8R"__(
        {
            let test = new TestType()
            test.value = g_add_value;
            test.print_value();
            test.add_to(TestType.add(7, 7));
            test.print_value();

            TestType.static_value = 114514;
            TestType.print_static_value();
        }
    )__");

    // gc
    isolate.gc(true);

    // trigger shutdown
    SKR_LOG_INFO(u8"==========================shutdown==========================");

    // shutdown
    context.shutdown();
    isolate.shutdown();
    shutdown_v8();

    return 0;
}