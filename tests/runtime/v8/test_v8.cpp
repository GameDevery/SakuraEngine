#include "SkrV8/v8_isolate.hpp"
#include "SkrV8/v8_context.hpp"
#include "SkrCore/exec_static.hpp"
#include "SkrRTTR/export/export_builder.hpp"
#include "SkrRTTR/rttr_traits.hpp"
#include "SkrRTTR/type.hpp"
#include "SkrCore/log.hpp"

// test type
struct TestType : public ::skr::rttr::ScriptbleObject {
    TestType() { SKR_LOG_FMT_INFO(u8"call ctor"); }
    TestType(int32_t v)
        : value(v)
    {
        SKR_LOG_FMT_INFO(u8"call ctor with param {}", v);
    }
    ~TestType()
    {
        SKR_LOG_FMT_INFO(u8"call dtor");
    }

    // field
    int32_t value;

    // method
    void print_value() const
    {
        SKR_LOG_FMT_INFO(u8"print value: {}", value);
    }

    // static field
    static int32_t static_value;

    // static method
    static void print_static_value()
    {
        SKR_LOG_FMT_INFO(u8"print static value: {}", static_value);
    }
};
int32_t TestType::static_value = 0;
SKR_RTTR_TYPE(TestType, "9f7696d7-4c20-4b11-84eb-7124b666c56e");
SKR_EXEC_STATIC_CTOR
{
    using namespace skr::rttr;
    register_type_loader(type_id_of<TestType>(), [](Type* type) {
        type->build_record([&](RecordData* data){
            RecordBuilder<TestType> rb{ data };
            // basic
            rb.basic_info();

            // bases
            rb.bases<::skr::rttr::ScriptbleObject>();
            
            // ctor
            rb.ctor<uint32_t>();
            rb.flag(ERecordFlag::ScriptNewable | ERecordFlag::ScriptVisible);
            
            // add field
            {
                auto fb = rb.field<&TestType::value>(u8"value");
                fb.flag(EFieldFlag::ScriptVisible);
            }

            // add method
            {
                auto mb = rb.method<&TestType::print_value>(u8"print_value");
                mb.flag(EMethodFlag::ScriptVisible);
            }

            // add static field
            {
                auto sfb = rb.static_field<&TestType::static_value>(u8"static_value");
                sfb.flag(EStaticFieldFlag::ScriptVisible);
            }

            // add static method
            {
                auto smb = rb.static_method<&TestType::print_static_value>(u8"print_static_value");
                smb.flag(EStaticMethodFlag::ScriptVisible);
            }
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

    // exec code
    context.exec_script(u8R"__(
        {
            let test = new TestType()
            let test_with_value_ctor = new TestType(114514);
            test_with_value_ctor.value = 114;
            let new_test = new TestType(test_with_value_ctor.value + 6); 
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