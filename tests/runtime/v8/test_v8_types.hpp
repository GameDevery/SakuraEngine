#include "SkrCore/log.hpp"
#include <SkrRTTR/scriptble_object.hpp>
#ifndef __meta__
    #include "V8Test/test_v8_types.generated.h"
#endif

// TODO. test string

// basic object
namespace test_v8
{
sreflect_struct(guid = "9099f452-beab-482b-a9c8-0f582bd7f5b4" rttr = @full)
sscript_visible sscript_newable
BasicObject : public ::skr::ScriptbleObject {
    SKR_GENERATE_BODY()

    sscript_visible
    BasicObject() { test_ctor_value = 114514; }
    sscript_visible
    BasicObject(int32_t v) { test_ctor_value = v; }

    static int32_t test_ctor_value;

    // method
    sscript_visible
    void test_method(int32_t v) { test_method_v = v; }
    int32_t test_method_v = 0;

    // field
    sscript_visible
    void assign_method_v_to_field() { test_field = test_method_v; }
    sscript_visible
    uint32_t test_field = 0;

    // static method
    sscript_visible
    static void test_static_method(int64_t v) { test_static_method_v = v; }
    static int64_t test_static_method_v;

    // static field
    sscript_visible
    static uint64_t test_static_field;

    // test overload
    sscript_visible
    void test_overload(int32_t v) { overload_int = v; }
    sscript_visible
    void test_overload(skr::String v) { overload_str = v; }
    int32_t overload_int = 0;
    skr::String overload_str = u8"";

    // test property
    sscript_visible sscript_getter(test_prop)
    int32_t get_test_prop() 
    { 
        ++test_prop_get_count;  
        return test_prop; 
    }
    sscript_visible sscript_setter(test_prop)
    void set_test_prop(int32_t v) 
    {
        ++test_prop_set_count; 
        test_prop = v; 
    }
    int32_t test_prop = 0;
    int32_t test_prop_get_count = 0;
    int32_t test_prop_set_count = 0;
};

sreflect_struct(guid = "549b208d-ca2b-44e1-a705-ef95e0c607b5" rttr = @full)
sscript_visible sscript_newable
InheritObject : public BasicObject {
    SKR_GENERATE_BODY()

    sscript_visible
    InheritObject() { test_ctor_value = 1919810; }

    sscript_visible
    void inherit_method(int32_t v) { test_field = v * 100; }
};
}

// basic value
namespace test_v8
{
sreflect_struct(guid = "ecaf33a3-0c3f-46b5-87aa-ba895fa6c38f" rttr = @full)
sscript_visible sscript_newable
BasicValue {
    sscript_visible
    BasicValue() { test_ctor_value = 114514; }
    sscript_visible
    BasicValue(int32_t v) { test_ctor_value = v; }

    static int32_t test_ctor_value;

    // method
    sscript_visible
    void test_method(int32_t v) { test_method_v = v; }
    int32_t test_method_v = 0;

    // field
    sscript_visible
    void assign_method_v_to_field() { test_field = test_method_v; }
    sscript_visible
    uint32_t test_field = 0;

    // static method
    sscript_visible
    static void test_static_method(int64_t v) { test_static_method_v = v; }
    static int64_t test_static_method_v;

    // static field
    sscript_visible
    static uint64_t test_static_field;

    // test overload
    sscript_visible
    void test_overload(int32_t v) { overload_int = v; }
    sscript_visible
    void test_overload(skr::String v) { overload_str = v; }
    int32_t overload_int = 0;
    skr::String overload_str = u8"";

    // test property
    sscript_visible sscript_getter(test_prop)
    int32_t get_test_prop() 
    { 
        ++test_prop_get_count;  
        return test_prop; 
    }
    sscript_visible sscript_setter(test_prop)
    void set_test_prop(int32_t v) 
    {
        ++test_prop_set_count; 
        test_prop = v; 
    }
    int32_t test_prop = 0;
    int32_t test_prop_get_count = 0;
    int32_t test_prop_set_count = 0;
};

sreflect_struct(guid = "2aeb519b-9b8e-4561-b185-81429dd1dd2e" rttr = @full)
sscript_visible sscript_newable
InheritValue : public BasicValue {
    sscript_visible
    InheritValue() { test_ctor_value = 1919810; }

    sscript_visible
    void inherit_method(int32_t v) { test_field = v * 100; }
};
}

// basic mappint
namespace test_v8
{
sreflect_struct(guid = "3fcd040d-e180-4802-996d-d67417e45f7e" rttr = @full)
sscript_visible sscript_mapping
BasicMapping {
    sscript_visible
    double x = 0;
    sscript_visible
    double y = 0;
    sscript_visible
    double z = 0;
};
sreflect_struct(guid = "b9b953e3-b63b-49bf-8d34-2dba7ff49dd2" rttr = @full)
sscript_visible sscript_mapping
InheritMapping : BasicMapping {
    sscript_visible
    double w = 0;
};
sreflect_struct(guid = "06e9fa99-9432-43fa-ba55-d22737a7de60" rttr = @full)
sscript_visible
BasicMappingHelper 
{
    sscript_visible
    static BasicMapping basic_value;
    sscript_visible
    static InheritMapping inherit_value;

    sscript_visible
    static void set(BasicMapping v) { basic_value = v; }
    sscript_visible
    static void set(InheritMapping v) { inherit_value = v; }
};
}

// basic enum
namespace test_v8
{
sreflect_enum_class(guid = "56611484-25f9-491b-b85e-08f634839938")
sscript_visible
BasicEnum : int32_t {
    Value1 sscript_visible = 0,
    Value2 sscript_visible = 1,
    Value3 sscript_visible = 114514,
    Value4 sscript_visible = 3,
    Value5 sscript_visible = 8848,
};

sreflect_struct(guid = "26f991d3-7030-4b04-af12-0e1f0d93bba6" rttr = @full)
sscript_visible
BasicEnumHelper {
    sscript_visible
    static BasicEnum test_value;
    sscript_visible
    static skr::String test_name;
};
}

// param flag
namespace test_v8
{
sreflect_struct(guid = "7d514abe-a7f9-4512-a94f-4f3e9eb19d94")
sscript_visible sscript_newable
ParamFlagTestValue {
    sscript_visible
    ParamFlagTestValue() = default;

    sscript_visible
    skr::String value;
};

sreflect_struct(guid = "d74058a0-24c4-4f86-b062-299674e3df53")
sscript_visible
ParamFlagTest {
    sscript_visible
    static skr::String test_value;

    sscript_visible
    static void test_pure_out(sparam_out skr::String& v) { v = u8"mambo"; }

    sscript_visible
    static void test_inout(sparam_inout skr::String& v) { v.append(u8" mambo"); }

    sscript_visible
    static skr::String test_multi_out(sparam_out skr::String& v1, sparam_out skr::String& v2) 
    { 
        v1 = u8"out"; 
        v2 = u8"mambo"; 
        return u8"mamba";
    }
    sscript_visible
    static skr::String test_multi_inout(sparam_inout skr::String& v1, sparam_inout skr::String& v2)
    {
        v1.append(u8"out");
        v2.append(u8"mambo");
        return u8"mamba";
    }

    sscript_visible
    static void test_value_pure_out(sparam_out ParamFlagTestValue& v)
    {
        v.value = u8"mambo";
    }
    sscript_visible
    static void test_value_inout(sparam_inout ParamFlagTestValue& v)
    {
        v.value.append(u8" baka");
    }
};
}