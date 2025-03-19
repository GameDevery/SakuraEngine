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
)
sscript_visible sscript_mapping
float3 {
    sscript_visible
    float x = 0;
    sscript_visible
    float y = 0;
    sscript_visible
    float z = 0;
};

sreflect_struct(
    guid = "a2f3b0d4-1c5e-4b8f-8c7d-6a9e0f1b2c3d"
    rttr = @full
)
sscript_visible sscript_mapping
Box3 {
    sscript_visible
    float3 min = {0, 0, 0};
    sscript_visible
    float3 max = {0, 0, 0};
};

sreflect_struct(
    guid = "6563e112-1be3-45d4-8c44-8d5e3ed9b3fd"
    rttr = @full
)
sscript_visible sscript_mapping
Box3Offset : Box3 {
    sscript_visible
    float3 offset = {0, 0, 0};
};

sreflect_struct(
    guid = "caf11e29-c119-4685-9845-cb17fd2cb119";
    rttr = @full;
)
sscript_visible sscript_newable
TestType : public ::skr::ScriptbleObject {
    SKR_GENERATE_BODY()

    sscript_visible
    TestType() { SKR_LOG_FMT_INFO(u8"call ctor"); }
    ~TestType()
    {
        SKR_LOG_FMT_INFO(u8"call dtor");
    }

    // field
    sscript_visible
    int32_t value;

    // method
    sscript_visible
    void print_value() const
    {
        SKR_LOG_FMT_INFO(u8"print value: {}", value);
    }
    sscript_visible
    void add_to(int32_t v)
    {
        value += v;
    }

    // static field
    sscript_visible
    static int32_t static_value;

    // static method
    sscript_visible
    static void print_static_value()
    {
        SKR_LOG_FMT_INFO(u8"print static value: {}", static_value);
    }
    sscript_visible
    static int32_t add(int32_t a, int32_t b)
    {
        return a + b;
    }

    // box type
    sscript_visible
    float3 pos;
    sscript_visible
    Box3Offset box;
    
    // overload
    sscript_visible
    static void print(const float3& pos)
    {
        SKR_LOG_FMT_INFO(u8"print pos: {{{}, {}, {}}}", pos.x, pos.y, pos.z);
    }
    sscript_visible
    static void print(const Box3Offset& box)
    {
        SKR_LOG_FMT_INFO(u8"print box: min: {{{}, {}, {}}} max: {{{}, {}, {}}} offset: {{{}, {}, {}}}", 
            box.min.x, box.min.y, box.min.z, 
            box.max.x, box.max.y, box.max.z,
            box.offset.x, box.offset.y, box.offset.z
        );
    }

    // getter & setter
    sscript_visible sscript_getter(prop_fuck)
    skr::String get_fuck() const 
    {
        SKR_LOG_FMT_INFO(u8"get fuck"); 
        return u8"fuck"; 
    }
    sscript_visible sscript_setter(prop_fuck)
    void set_fuck(skr::String v)
    {
        SKR_LOG_FMT_INFO(u8"set fuck: {}"); 
    }

    // static getter & static setter
    sscript_visible sscript_getter(prop_shit)
    static int32_t get_shit()
    {
        SKR_LOG_FMT_INFO(u8"get shit");
        return 114514;
    }
    sscript_visible sscript_setter(prop_shit)
    static void set_shit(int32_t v)
    {
        SKR_LOG_FMT_INFO(u8"set shit {}", v);
    }

    // inout param
    sscript_visible
    void get_shit(sparam_out skr::String& out)
    {
        out = u8"shit";
    }
};
};