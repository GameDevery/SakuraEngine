#include "SkrTestFramework/framework.hpp"
#include "SkrCore/log.hpp"
#include "SkrRTTR/generic/generic_optional.hpp"
#include "SkrContainersDef/optional.hpp"

TEST_CASE("test generic container")
{
    using namespace skr;

    // test optional
    SUBCASE("test optional")
    {
        Optional<bool>    opt_bool;
        Optional<int16_t> opt_int16;
        Optional<int32_t> opt_int32;
        Optional<int64_t> opt_int64;
        Optional<float>   opt_float;
        Optional<double>  opt_double;
        Optional<GUID>    opt_guid;

        RC<GenericOptional> generic_opt_bool   = build_generic(type_signature_of<Optional<bool>>()).cast_static<GenericOptional>();
        RC<GenericOptional> generic_opt_int16  = build_generic(type_signature_of<Optional<int16_t>>()).cast_static<GenericOptional>();
        RC<GenericOptional> generic_opt_int32  = build_generic(type_signature_of<Optional<int32_t>>()).cast_static<GenericOptional>();
        RC<GenericOptional> generic_opt_int64  = build_generic(type_signature_of<Optional<int64_t>>()).cast_static<GenericOptional>();
        RC<GenericOptional> generic_opt_float  = build_generic(type_signature_of<Optional<float>>()).cast_static<GenericOptional>();
        RC<GenericOptional> generic_opt_double = build_generic(type_signature_of<Optional<double>>()).cast_static<GenericOptional>();
        RC<GenericOptional> generic_opt_guid   = build_generic(type_signature_of<Optional<GUID>>()).cast_static<GenericOptional>();

        // check address
        REQUIRE_EQ(&opt_bool.value(), generic_opt_bool->value_ptr(&opt_bool));
        REQUIRE_EQ(&opt_int16.value(), generic_opt_int16->value_ptr(&opt_int16));
        REQUIRE_EQ(&opt_int32.value(), generic_opt_int32->value_ptr(&opt_int32));
        REQUIRE_EQ(&opt_int64.value(), generic_opt_int64->value_ptr(&opt_int64));
        REQUIRE_EQ(&opt_float.value(), generic_opt_float->value_ptr(&opt_float));
        REQUIRE_EQ(&opt_double.value(), generic_opt_double->value_ptr(&opt_double));
        REQUIRE_EQ(&opt_guid.value(), generic_opt_guid->value_ptr(&opt_guid));

        // check has_value
        REQUIRE_FALSE(generic_opt_bool->has_value(&opt_bool));
        REQUIRE_FALSE(generic_opt_int16->has_value(&opt_int16));
        REQUIRE_FALSE(generic_opt_int32->has_value(&opt_int32));
        REQUIRE_FALSE(generic_opt_int64->has_value(&opt_int64));
        REQUIRE_FALSE(generic_opt_float->has_value(&opt_float));
        REQUIRE_FALSE(generic_opt_double->has_value(&opt_double));
        REQUIRE_FALSE(generic_opt_guid->has_value(&opt_guid));

        // do assign
        opt_bool   = true;
        opt_int16  = 1;
        opt_int32  = 2;
        opt_int64  = 3;
        opt_float  = 4.0f;
        opt_double = 5.0;
        opt_guid   = u8"051b41eb-2f26-4509-9032-0ca30d9b1990"_guid;

        // check assigned has value
        REQUIRE(generic_opt_bool->has_value(&opt_bool));
        REQUIRE(generic_opt_int16->has_value(&opt_int16));
        REQUIRE(generic_opt_int32->has_value(&opt_int32));
        REQUIRE(generic_opt_int64->has_value(&opt_int64));
        REQUIRE(generic_opt_float->has_value(&opt_float));
        REQUIRE(generic_opt_double->has_value(&opt_double));
        REQUIRE(generic_opt_guid->has_value(&opt_guid));

        // generic reset
        generic_opt_bool->reset(&opt_bool);
        generic_opt_int16->reset(&opt_int16);
        generic_opt_int32->reset(&opt_int32);
        generic_opt_int64->reset(&opt_int64);
        generic_opt_float->reset(&opt_float);
        generic_opt_double->reset(&opt_double);
        generic_opt_guid->reset(&opt_guid);

        // check reset has value
        REQUIRE_FALSE(opt_bool.has_value());
        REQUIRE_FALSE(opt_int16.has_value());
        REQUIRE_FALSE(opt_int32.has_value());
        REQUIRE_FALSE(opt_int64.has_value());
        REQUIRE_FALSE(opt_float.has_value());
        REQUIRE_FALSE(opt_double.has_value());
        REQUIRE_FALSE(opt_guid.has_value());

        // assign value
        bool    bool_val   = false;
        int16_t int16_val  = 10;
        int32_t int32_val  = 20;
        int64_t int64_val  = 30;
        float   float_val  = 40.5f;
        double  double_val = 50.5;
        auto    guid_val   = u8"a1b2c3d4-e5f6-7890-abcd-123456789abc"_guid;

        // generic assign
        generic_opt_bool->assign_value(&opt_bool, &bool_val);
        generic_opt_int16->assign_value(&opt_int16, &int16_val);
        generic_opt_int32->assign_value(&opt_int32, &int32_val);
        generic_opt_int64->assign_value(&opt_int64, &int64_val);
        generic_opt_float->assign_value(&opt_float, &float_val);
        generic_opt_double->assign_value(&opt_double, &double_val);
        generic_opt_guid->assign_value(&opt_guid, &guid_val);

        // check generic assigned has value
        REQUIRE(generic_opt_bool->has_value(&opt_bool));
        REQUIRE(generic_opt_int16->has_value(&opt_int16));
        REQUIRE(generic_opt_int32->has_value(&opt_int32));
        REQUIRE(generic_opt_int64->has_value(&opt_int64));
        REQUIRE(generic_opt_float->has_value(&opt_float));
        REQUIRE(generic_opt_double->has_value(&opt_double));
        REQUIRE(generic_opt_guid->has_value(&opt_guid));

        // check generic assigned values are correct
        REQUIRE_EQ(opt_bool.value(), bool_val);
        REQUIRE_EQ(opt_int16.value(), int16_val);
        REQUIRE_EQ(opt_int32.value(), int32_val);
        REQUIRE_EQ(opt_int64.value(), int64_val);
        REQUIRE_EQ(opt_float.value(), float_val);
        REQUIRE_EQ(opt_double.value(), double_val);
        REQUIRE_EQ(opt_guid.value(), guid_val);

        // reset all
        opt_bool.reset();
        opt_int16.reset();
        opt_int32.reset();
        opt_int64.reset();
        opt_float.reset();
        opt_double.reset();
        opt_guid.reset();

        // cache values
        bool    cached_bool_val   = bool_val;
        int16_t cached_int16_val  = int16_val;
        int32_t cached_int32_val  = int32_val;
        int64_t cached_int64_val  = int64_val;
        float   cached_float_val  = float_val;
        double  cached_double_val = double_val;
        auto    cached_guid_val   = guid_val;

        // generic assign move
        generic_opt_bool->assign_value_move(&opt_bool, &bool_val);
        generic_opt_int16->assign_value_move(&opt_int16, &int16_val);
        generic_opt_int32->assign_value_move(&opt_int32, &int32_val);
        generic_opt_int64->assign_value_move(&opt_int64, &int64_val);
        generic_opt_float->assign_value_move(&opt_float, &float_val);
        generic_opt_double->assign_value_move(&opt_double, &double_val);
        generic_opt_guid->assign_value_move(&opt_guid, &guid_val);

        // check generic assigned move values are correct
        REQUIRE(generic_opt_bool->has_value(&opt_bool));
        REQUIRE(generic_opt_int16->has_value(&opt_int16));
        REQUIRE(generic_opt_int32->has_value(&opt_int32));
        REQUIRE(generic_opt_int64->has_value(&opt_int64));
        REQUIRE(generic_opt_float->has_value(&opt_float));
        REQUIRE(generic_opt_double->has_value(&opt_double));
        REQUIRE(generic_opt_guid->has_value(&opt_guid));

        // check generic assigned move values are correct
        REQUIRE_EQ(opt_bool.value(), cached_bool_val);
        REQUIRE_EQ(opt_int16.value(), cached_int16_val);
        REQUIRE_EQ(opt_int32.value(), cached_int32_val);
        REQUIRE_EQ(opt_int64.value(), cached_int64_val);
        REQUIRE_EQ(opt_float.value(), cached_float_val);
        REQUIRE_EQ(opt_double.value(), cached_double_val);
        REQUIRE_EQ(opt_guid.value(), cached_guid_val);

        SUBCASE("nested")
        {
            Optional<Optional<int32_t>> nested_opt_int32;
            RC<GenericOptional>         generic_nested_opt_int32 = build_generic(type_signature_of<Optional<Optional<int32_t>>>()).cast_static<GenericOptional>();

            Optional<int32_t> inner_opt_int32;
            inner_opt_int32 = 114514;

            generic_nested_opt_int32->assign_value_move(&nested_opt_int32, &inner_opt_int32);

            REQUIRE(!inner_opt_int32.has_value());
            REQUIRE(nested_opt_int32.has_value());
            REQUIRE(nested_opt_int32.value().has_value());
            REQUIRE_EQ(nested_opt_int32.value().value(), 114514);
        }
    }
} 