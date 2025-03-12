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
    guid = "a2f3b0d4-1c5e-4b8f-8c7d-6a9e0f1b2c3d"
    rttr = @full
    rttr.flags = ["ScriptBox"]
)
Box3 {
    sattr(rttr.flags = ["ScriptVisible"])
    float3 min = {0, 0, 0};
    sattr(rttr.flags = ["ScriptVisible"])
    float3 max = {0, 0, 0};
};

sreflect_struct(
    guid = "6563e112-1be3-45d4-8c44-8d5e3ed9b3fd"
    rttr = @full
    rttr.flags = ["ScriptBox"]
)
Box3Offset : Box3 {
    sattr(rttr.flags = ["ScriptVisible"])
    float3 offset = {0, 0, 0};
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

    // box type
    sattr(rttr.flags += ["ScriptVisible"])
    float3 pos;
    sattr(rttr.flags += ["ScriptVisible"])
    Box3Offset box;
    
    // overload
    sattr(rttr.flags += ["ScriptVisible"])
    static void print(const float3& pos)
    {
        SKR_LOG_FMT_INFO(u8"print pos: {{{}, {}, {}}}", pos.x, pos.y, pos.z);
    }
    sattr(rttr.flags += ["ScriptVisible"])
    static void print(const Box3Offset& box)
    {
        SKR_LOG_FMT_INFO(u8"print box: min: {{{}, {}, {}}} max: {{{}, {}, {}}} offset: {{{}, {}, {}}}", 
            box.min.x, box.min.y, box.min.z, 
            box.max.x, box.max.y, box.max.z,
            box.offset.x, box.offset.y, box.offset.z
        );
    }
};
};