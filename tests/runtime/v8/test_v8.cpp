#include "SkrTestFramework/framework.hpp"
#include "SkrV8/ts_define_exporter.hpp"
#include "SkrV8/v8_isolate.hpp"
#include "SkrV8/v8_context.hpp"
#include "SkrRTTR/rttr_traits.hpp"
#include "SkrCore/log.hpp"
#include "test_v8_types.hpp"

TEST_CASE("test v8")
{
    using namespace skr;

    V8Isolate isolate;

    // init
    init_v8();
    isolate.init();

    SUBCASE("basic object")
    {
        V8Context context(&isolate);
        context.init();

        // register context
        context.register_type<test_v8::BasicObject>();
        context.register_type<test_v8::InheritObject>();

        // set global
        test_v8::BasicObject* obj = SkrNew<test_v8::BasicObject>();
        context.set_global(u8"test_obj", obj);

        // test method
        context.exec_script(u8"test_obj.test_method(114514)");
        REQUIRE_EQ(obj->test_method_v, 114514);

        // test field
        context.exec_script(u8"test_obj.test_field = 1919810");
        REQUIRE_EQ(obj->test_field, 1919810);
        obj->test_field = 44554;
        context.exec_script(u8"test_obj.test_method(test_obj.test_field)");
        REQUIRE_EQ(obj->test_method_v, 44554);

        // test static method
        context.exec_script(u8"BasicObject.test_static_method(1919810)");
        REQUIRE_EQ(test_v8::BasicObject::test_static_method_v, 1919810);

        // test static field
        context.exec_script(u8"BasicObject.test_static_field = 114514");
        REQUIRE_EQ(test_v8::BasicObject::test_static_field, 114514);
        test_v8::BasicObject::test_static_field = 44544;
        context.exec_script(u8"test_obj.test_method(Number(BasicObject.test_static_field))");
        REQUIRE_EQ(obj->test_method_v, 44544);

        // test overload
        context.exec_script(u8"test_obj.test_overload(114514)");
        REQUIRE_EQ(obj->overload_int, 114514);
        context.exec_script(u8"test_obj.test_overload('1919810')");
        REQUIRE_EQ(obj->overload_str, u8"1919810");

        // test property
        context.exec_script(u8"test_obj.test_prop = 1");
        context.exec_script(u8"test_obj.test_prop = 2");
        context.exec_script(u8"test_obj.test_prop = 3");
        context.exec_script(u8"test_obj.test_prop = 114514");
        REQUIRE_EQ(obj->test_prop, 114514);
        REQUIRE_EQ(obj->test_prop_set_count, 4);
        context.exec_script(u8"let prop_v = test_obj.test_prop");
        context.exec_script(u8"BasicObject.test_static_field = prop_v");
        REQUIRE_EQ(obj->test_prop_get_count, 1);
        REQUIRE_EQ(test_v8::BasicObject::test_static_field, 114514);
        obj->test_prop = 10086;
        context.exec_script(u8"test_obj.test_method(test_obj.test_prop)");
        REQUIRE_EQ(obj->test_method_v, 10086);
        REQUIRE_EQ(obj->test_prop_get_count, 2);

        // test new
        context.exec_script(u8"let new_obj_1 = new BasicObject()");
        REQUIRE_EQ(test_v8::BasicObject::test_ctor_value, 114514);
        context.exec_script(u8"let new_obj_2 = new BasicObject(568798)");
        REQUIRE_EQ(test_v8::BasicObject::test_ctor_value, 568798);

        // test inherit
        context.exec_script(u8"let inherit_obj = new InheritObject()");
        context.exec_script(u8"inherit_obj.inherit_method(114514)");
        REQUIRE_EQ(test_v8::BasicObject::test_ctor_value, 1919810);
        {
            auto result = context.exec_script(u8"inherit_obj.test_field");
            REQUIRE(!result.is_empty());
            auto v = result.get<uint32_t>();
            REQUIRE(v.has_value());
            REQUIRE_EQ(v.value(), 114514 * 100);
        }
        context.exec_script(u8"inherit_obj.test_method(114514)");
        context.exec_script(u8"inherit_obj.assign_method_v_to_field()");
        {
            auto result = context.exec_script(u8"inherit_obj.test_field");
            REQUIRE(!result.is_empty());
            auto v = result.get<uint32_t>();
            REQUIRE(v.has_value());
            REQUIRE_EQ(v.value(), 114514);
        }
        // {
        //     auto result = context.exec_script(u8"inherit_obj instanceof InheritObject");
        //     REQUIRE(!result.is_empty());
        //     auto v = result.get<bool>();
        //     REQUIRE(v.has_value());
        //     REQUIRE(v.value() == true);
        // }
        // {
        //     auto result = context.exec_script(u8"inherit_obj instanceof BasicObject");
        //     REQUIRE(!result.is_empty());
        //     auto v = result.get<bool>();
        //     REQUIRE(v.has_value());
        //     REQUIRE(v.value() == true);
        // }

        // test lifetime
        SkrDelete(obj);
        context.exec_script(u8R"__(
            let error = false
            try {
                test_obj.test_method(114514);
            } catch (e) {
                error = true;
            }
        )__");
        {
            auto result = context.exec_script(u8"error");
            REQUIRE(!result.is_empty());
            auto v = result.get<bool>();
            REQUIRE(v.has_value());
            REQUIRE(v.value() == true);
        }


        context.shutdown();
    }

    // shutdown
    isolate.shutdown();
    shutdown_v8();
}