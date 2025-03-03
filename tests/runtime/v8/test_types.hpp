#include "SkrCore/log.hpp"
#include <SkrRTTR/scriptble_object.hpp>
#ifndef __meta__
    #include "V8Test/test_types.generated.h"
#endif

namespace test_v8
{
sreflect_struct(
    "guid": "caf11e29-c119-4685-9845-cb17fd2cb119",
    "rttr": "full"
)
sattr("rttr::flags+": ["ScriptVisible", "ScriptNewable"])
TestType : public ::skr::rttr::ScriptbleObject {
    SKR_GENERATE_BODY()

    TestType() { SKR_LOG_FMT_INFO(u8"call ctor"); }
    ~TestType()
    {
        SKR_LOG_FMT_INFO(u8"call dtor");
    }

    // field
    sattr("rttr::flags+": ["ScriptVisible"])
    int32_t value;

    // method
    sattr("rttr::flags+": ["ScriptVisible"])
    void print_value() const
    {
        SKR_LOG_FMT_INFO(u8"print value: {}", value);
    }
    sattr("rttr::flags+": ["ScriptVisible"])
    void add_to(int32_t v)
    {
        value += v;
    }

    // static field
    sattr("rttr::flags+": ["ScriptVisible"])
    static int32_t static_value;

    // static method
    sattr("rttr::flags+": ["ScriptVisible"])
    static void print_static_value()
    {
        SKR_LOG_FMT_INFO(u8"print static value: {}", static_value);
    }
    sattr("rttr::flags+": ["ScriptVisible"])
    static int32_t add(int32_t a, int32_t b)
    {
        return a + b;
    }
};
};