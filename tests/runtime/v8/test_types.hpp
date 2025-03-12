#include "SkrCore/log.hpp"
#include <SkrRTTR/scriptble_object.hpp>
#ifndef __meta__
    #include "V8Test/test_types.generated.h"
#endif

namespace test_v8
{
sreflect_struct(
    guid = "612a176d-e2c8-4b22-89fa-2370d7ec5e44"
    rttr = @full
    rttr.flags = ["ScriptBox"]
)
float3 {
    sattr(rttr.flags = ["ScriptVisible"])
    float x = 0;
    sattr(rttr.flags = ["ScriptVisible"])
    float y = 0;
    sattr(rttr.flags = ["ScriptVisible"])
    float z = 0;
};

sreflect_struct(
    guid = "caf11e29-c119-4685-9845-cb17fd2cb119";
    rttr = @full;
)
sattr(rttr.flags = ["ScriptVisible", "ScriptNewable"])
TestType : public ::skr::rttr::ScriptbleObject {
    SKR_GENERATE_BODY()

    TestType() { SKR_LOG_FMT_INFO(u8"call ctor"); }
    ~TestType()
    {
        SKR_LOG_FMT_INFO(u8"call dtor");
    }

    // field
    sattr(rttr.flags +=  ["ScriptVisible"])
    int32_t value;

    // method
    sattr(rttr.flags += ["ScriptVisible"])
    void print_value() const
    {
        SKR_LOG_FMT_INFO(u8"print value: {}", value);
    }
    sattr(rttr.flags += ["ScriptVisible"])
    void add_to(int32_t v)
    {
        value += v;
    }

    // static field
    sattr(rttr.flags += ["ScriptVisible"])
    static int32_t static_value;

    // static method
    sattr(rttr.flags += ["ScriptVisible"])
    static void print_static_value()
    {
        SKR_LOG_FMT_INFO(u8"print static value: {}", static_value);
    }
    sattr(rttr.flags += ["ScriptVisible"])
    static int32_t add(int32_t a, int32_t b)
    {
        return a + b;
    }

    // mapping type
    sattr(rttr.flags += ["ScriptVisible"])
    float3 pos;
    sattr(rttr.flags += ["ScriptVisible"])
    void print_pos() const
    {
        SKR_LOG_FMT_INFO(u8"print pos: {{{}, {}, {}}}", pos.x, pos.y, pos.z);
    }
};
};