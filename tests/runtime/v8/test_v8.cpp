#include "SkrV8/v8_isolate.hpp"
#include "SkrV8/v8_context.hpp"
#include "SkrCore/exec_static.hpp"
#include "SkrRTTR/export/export_builder.hpp"
#include "SkrRTTR/rttr_traits.hpp"
#include "SkrRTTR/type.hpp"
#include "SkrCore/log.hpp"

// test type
struct TestType {
    TestType() { SKR_LOG_FMT_INFO(u8"call ctor"); }
    TestType(int32_t v)
        : value(v)
    {
        SKR_LOG_FMT_INFO(u8"call ctor with param {}", v);
    }
    int32_t value;
};
SKR_RTTR_TYPE(TestType, "9f7696d7-4c20-4b11-84eb-7124b666c56e");
SKR_EXEC_STATIC_CTOR
{
    using namespace skr::rttr;
    register_type_loader(type_id_of<TestType>(), [](Type* type) {
        type->build_record([&](RecordData* data){
            RecordBuilder<TestType> builder{ data };
            builder.basic_info();
            builder.ctor<uint32_t>();
            builder.field<&TestType::value>(u8"value");
        });
    });
};

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
    isolate.make_record_template(skr::rttr::type_of<TestType>());

    // inject into context
    context.install_templates();

    // exec code
    context.exec_script(u8R"__(
        let test = new TestType()
        let test_with_value_ctor = new TestType(114514);
        test_with_value_ctor.value = 114;
        let new_test = new TestType(test_with_value_ctor.value + 6); 
    )__");

    // shutdown
    context.shutdown();
    isolate.shutdown();
    shutdown_v8();
}