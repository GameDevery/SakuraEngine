#pragma once
#include "SkrCore/log.hpp"
#include <SkrRTTR/script/scriptble_object.hpp>
#ifndef __meta__
    #include "test_v8_types_new.generated.h"
#endif

// error export test
namespace test_v8
{
sreflect_struct(guid = "a1b2c3d4-e5f6-7890-abcd-ef1234567890")
sscript_visible sscript_newable
ErrorExport {
    sscript_visible
    int32_t* primitive_ref_field = {};

    sscript_visible
    skr::StringView string_view_field = {};

    sscript_visible
    ErrorExport() {}

    sscript_visible
    ErrorExport(int32_t a) {}
    
    sscript_visible
    void primitive_pointer_param(int32_t* a) {}
};
} // namespace test_v8

// basic object
namespace test_v8
{
sreflect_struct(guid = "9099f452-beab-482b-a9c8-0f582bd7f5b4" rttr = @full)
sscript_visible sscript_newable
BasicObject : public ::skr::ScriptbleObject {
    SKR_GENERATE_BODY(BasicObject)

    sscript_visible
    BasicObject() { test_ctor_value = 114514; }

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
    SKR_GENERATE_BODY(InheritObject)

    sscript_visible
    InheritObject() { test_ctor_value = 1919810; }

    sscript_visible
    void inherit_method(int32_t v) { test_field = v * 100; }
};
}