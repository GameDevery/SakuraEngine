#include "SkrTestFramework/framework.hpp"

#include <SkrCore/delegate/delegate.hpp>
#include <random>

struct TestBindMember {
    int32_t value = 0;
    
    int32_t get_value() const
    {
        return value;
    }

    int32_t get_value_mul_10()
    {
        return value * 10;
    }
};

TEST_CASE("test delegate")
{
    using Delegate = skr::Delegate<int32_t()>;
    std::random_device                     rand_device;
    std::default_random_engine             rand_engine(rand_device());
    std::uniform_int_distribution<int32_t> rand_dis{};

    SUBCASE("bind test")
    {
        Delegate d;

        // empty
        auto empty_result = d.invoke();
        REQUIRE_EQ(empty_result.has_value(), false);

        // static
        d.bind_static(+[]() {
            return 114514;
        });
        auto static_result = d.invoke();
        REQUIRE_EQ(static_result.has_value(), true);
        REQUIRE_EQ(static_result.value(), 114514);

        // member
        TestBindMember test_obj;
        d.bind_member<&TestBindMember::get_value>(&test_obj);
        test_obj.value = 11451400;
        auto member_result = d.invoke();
        REQUIRE_EQ(member_result.has_value(), true);    
        REQUIRE_EQ(member_result.value(), 11451400);
        d.bind_member<&TestBindMember::get_value_mul_10>(&test_obj);
        auto member_result_mul_10 = d.invoke();
        REQUIRE_EQ(member_result_mul_10.has_value(), true);
        REQUIRE_EQ(member_result_mul_10.value(), 114514000);

        // lambda
        int32_t test_lambda_value = 1145;
        d.bind_lambda([&test_lambda_value]() {
            return test_lambda_value;
        });
        test_lambda_value = 11451;
        auto lambda_result = d.invoke();
        REQUIRE_EQ(lambda_result.has_value(), true);
        REQUIRE_EQ(lambda_result.value(), 11451);

        // functor
        struct {
            int32_t operator()() {
                return 114;
            };
        } test_functor;
        d.bind_functor(test_functor);
        auto functor_result = d.invoke();
        REQUIRE_EQ(functor_result.has_value(), true);
        REQUIRE_EQ(functor_result.value(), 114);
    }

    SUBCASE("bind test use factory")
    {
        Delegate d;

        // empty
        auto empty_result = d.invoke();
        REQUIRE_EQ(empty_result.has_value(), false);

        // static
        d = Delegate::Static(+[]() {
            return 114514;
        });
        auto static_result = d.invoke();
        REQUIRE_EQ(static_result.has_value(), true);
        REQUIRE_EQ(static_result.value(), 114514);

        // member
        TestBindMember test_obj;
        d = Delegate::Member<&TestBindMember::get_value>(&test_obj);
        test_obj.value = 11451400;
        auto member_result = d.invoke();
        REQUIRE_EQ(member_result.has_value(), true);    
        REQUIRE_EQ(member_result.value(), 11451400);

        // lambda
        int32_t test_lambda_value = 1145;
        d = Delegate::Lambda([&test_lambda_value]() {
            return test_lambda_value;
        });
        test_lambda_value = 11451;
        auto lambda_result = d.invoke();
        REQUIRE_EQ(lambda_result.has_value(), true);
        REQUIRE_EQ(lambda_result.value(), 11451);

        // functor
        struct {
            int32_t operator()() {
                return 114;
            };
        } test_functor;
        d = Delegate::Functor(test_functor);
        auto functor_result = d.invoke();
        REQUIRE_EQ(functor_result.has_value(), true);
        REQUIRE_EQ(functor_result.value(), 114);
    }
}