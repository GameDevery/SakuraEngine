#include "SkrV8/v8_isolate.hpp"
#include "SkrV8/v8_context.hpp"
#include "SkrRTTR/rttr_traits.hpp"
#include "SkrCore/log.hpp"
#include "test_v8_types.hpp"

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
    context.register_type<test_v8::TestType>();
    context.register_type<test_v8::ETestEnum>();

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

            let fuck_str = test.prop_fuck
            test.prop_fuck = `"${fuck_str}, ${test.value}"`

            let shit_val = TestType.prop_shit
            TestType.prop_shit = shit_val * 100

            let oh_baka = test.get_oh_baka()
            let [mamba, out] = test.get_mamba_out()
            oh_baka = test.add_mambo(oh_baka)
            TestType.log(`"${oh_baka}" | "${mamba}" "${out}"`)

            let e_none = ETestEnum.None;
            let e_a = ETestEnum.A;
            let e_b = ETestEnum.B;
            let e_c = ETestEnum.C;
            TestType.log(`None: ${e_none}, A: ${e_a}, B: ${e_b}, C: ${e_c}`)
            TestType.log(`None: '${ETestEnum.to_string(e_none)}', A: '${ETestEnum.to_string(e_a)}', B: '${ETestEnum.to_string(e_b)}', C: '${ETestEnum.to_string(e_c)}'`)
            TestType.log(`C from string ${ETestEnum.from_string('C')}`)
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