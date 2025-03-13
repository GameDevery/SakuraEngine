#include "SkrV8/v8_isolate.hpp"
#include "SkrV8/v8_context.hpp"
#include "SkrRTTR/rttr_traits.hpp"
#include "SkrCore/log.hpp"
#include "test_types.hpp"

int32_t test_v8::TestType::static_value = 0;

int main(int argc, char* argv[])
{
    using namespace skr;

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

            test.pos = {x: 11, y: 45, z: 14};
            test.box = {
                min: {x: 1, y: 2, z: 3},
                max: {x: 4, y: 5, z: 6},
                offset: {x: 7, y: 8, z: 9},
            }

            const pos = test.pos
            const box = test.box
            TestType.print(pos);
            TestType.print(box);

            TestType.static_value = test.pos.x * 10000 + test.pos.y * 100 + test.pos.z;
            TestType.print_static_value();
        }
    )__");

    // gc
    isolate.gc(true);

    // trigger shutdown
    SKR_LOG_FMT_INFO(u8"==========================shutdown==========================");

    // shutdown
    context.shutdown();
    isolate.shutdown();
    shutdown_v8();

    return 0;
}