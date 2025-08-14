#include "SkrTestFramework/framework.hpp"
#include "SkrRTTR/rttr_traits.hpp"
#include "SkrCore/log.hpp"
#include "test_v8_types_new.hpp"
#include "SkrV8/v8_isolate.hpp"
#include "SkrV8/v8_context.hpp"

skr::V8Isolate isolate;

struct V8EnvGuard
{
    V8EnvGuard()
    {
        skr::init_v8();
        isolate.init();
    }
    ~V8EnvGuard()
    {
        isolate.shutdown();
        skr::shutdown_v8();
    }
} _guard{};

TEST_CASE("dump error export")
{
    return;

    auto* context = isolate.create_context();
    SKR_DEFER({ isolate.destroy_context(context); });

    SKR_LOG_FMT_ERROR(u8"Dump error export");
    context->build_export([](skr::V8VirtualModule& module) {
        module.register_type<test_v8::ErrorExport>(u8"");
    });
}

TEST_CASE("basic export test")
{
    using namespace skr;

    SUBCASE("basic object")
    {
        V8Context& context = *isolate.create_context();
        SKR_DEFER({ isolate.destroy_context(&context); });

        // register context
        context.build_export([](V8VirtualModule& module) {
            module.register_type<test_v8::BasicObject>(u8"");
            module.register_type<test_v8::InheritObject>(u8"");
        });

        // set global
        test_v8::BasicObject* obj = SkrNew<test_v8::BasicObject>();
        context.set_global(u8"test_obj", obj);

        // test method
        context.exec(u8"test_obj.test_method(114514)");
        REQUIRE_EQ(obj->test_method_v, 114514);

        // test field
        context.exec(u8"test_obj.test_field = 1919810");
        REQUIRE_EQ(obj->test_field, 1919810);
        obj->test_field = 44554;
        context.exec(u8"test_obj.test_method(test_obj.test_field)");
        REQUIRE_EQ(obj->test_method_v, 44554);

        // test static method
        context.exec(u8"BasicObject.test_static_method(1919810)");
        REQUIRE_EQ(test_v8::BasicObject::test_static_method_v, 1919810);

        // test static field
        context.exec(u8"BasicObject.test_static_field = 114514");
        REQUIRE_EQ(test_v8::BasicObject::test_static_field, 114514);
        test_v8::BasicObject::test_static_field = 44544;
        context.exec(u8"test_obj.test_method(Number(BasicObject.test_static_field))");
        REQUIRE_EQ(obj->test_method_v, 44544);

        // test property
        context.exec(u8"test_obj.test_prop = 1");
        context.exec(u8"test_obj.test_prop = 2");
        context.exec(u8"test_obj.test_prop = 3");
        context.exec(u8"test_obj.test_prop = 114514");
        REQUIRE_EQ(obj->test_prop, 114514);
        REQUIRE_EQ(obj->test_prop_set_count, 4);
        context.exec(u8"let prop_v = test_obj.test_prop");
        context.exec(u8"BasicObject.test_static_field = prop_v");
        REQUIRE_EQ(obj->test_prop_get_count, 1);
        REQUIRE_EQ(test_v8::BasicObject::test_static_field, 114514);
        obj->test_prop = 10086;
        context.exec(u8"test_obj.test_method(test_obj.test_prop)");
        REQUIRE_EQ(obj->test_method_v, 10086);
        REQUIRE_EQ(obj->test_prop_get_count, 2);

        // test new
        context.exec(u8"let new_obj_1 = new BasicObject()");
        REQUIRE_EQ(test_v8::BasicObject::test_ctor_value, 114514);

        // test inherit
        context.exec(u8"let inherit_obj = new InheritObject()");
        context.exec(u8"inherit_obj.inherit_method(114514)");
        REQUIRE_EQ(test_v8::BasicObject::test_ctor_value, 1919810);
        {
            auto result = context.exec(u8"inherit_obj.test_field");
            REQUIRE(!result.is_empty());
            auto v = result.get<uint32_t>();
            REQUIRE(v.has_value());
            REQUIRE_EQ(v.value(), 114514 * 100);
        }
        context.exec(u8"inherit_obj.test_method(114514)");
        context.exec(u8"inherit_obj.assign_method_v_to_field()");
        {
            auto result = context.exec(u8"inherit_obj.test_field");
            REQUIRE(!result.is_empty());
            auto v = result.get<uint32_t>();
            REQUIRE(v.has_value());
            REQUIRE_EQ(v.value(), 114514);
        }

        // test lifetime
        SkrDelete(obj);
        context.exec(u8R"__(
            let error = false
            try {
                test_obj.test_method(114514);
            } catch (e) {
                error = true;
            }
        )__");
        {
            auto result = context.exec(u8"error");
            REQUIRE(!result.is_empty());
            auto v = result.get<bool>();
            REQUIRE(v.has_value());
            REQUIRE(v.value() == true);
        }
    }
}