#include "SkrTestFramework/framework.hpp"
#include "SkrCore/log.hpp"
#include "SkrRTTR/generic/generic_optional.hpp"
#include "SkrContainersDef/optional.hpp"

TEST_CASE("test generic container")
{
    using namespace skr;

#if 0
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

        GenericOptional generic_opt_bool(&opt_bool, type_of<bool>());
        GenericOptional generic_opt_int16(&opt_int16, type_of<int16_t>());
        GenericOptional generic_opt_int32(&opt_int32, type_of<int32_t>());
        GenericOptional generic_opt_int64(&opt_int64, type_of<int64_t>());
        GenericOptional generic_opt_float(&opt_float, type_of<float>());
        GenericOptional generic_opt_double(&opt_double, type_of<double>());
        GenericOptional generic_opt_guid(&opt_guid, type_of<GUID>());

        // init
        REQUIRE(generic_opt_bool.init());
        REQUIRE(generic_opt_int16.init());
        REQUIRE(generic_opt_int32.init());
        REQUIRE(generic_opt_int64.init());
        REQUIRE(generic_opt_float.init());
        REQUIRE(generic_opt_double.init());
        REQUIRE(generic_opt_guid.init());

        // check address
        REQUIRE_EQ(&opt_bool.value(), generic_opt_bool.value_ptr());
        REQUIRE_EQ(&opt_int16.value(), generic_opt_int16.value_ptr());
        REQUIRE_EQ(&opt_int32.value(), generic_opt_int32.value_ptr());
        REQUIRE_EQ(&opt_int64.value(), generic_opt_int64.value_ptr());
        REQUIRE_EQ(&opt_float.value(), generic_opt_float.value_ptr());
        REQUIRE_EQ(&opt_double.value(), generic_opt_double.value_ptr());
        REQUIRE_EQ(&opt_guid.value(), generic_opt_guid.value_ptr());

        // check has_value
        REQUIRE_FALSE(generic_opt_bool.has_value());
        REQUIRE_FALSE(generic_opt_int16.has_value());
        REQUIRE_FALSE(generic_opt_int32.has_value());
        REQUIRE_FALSE(generic_opt_int64.has_value());
        REQUIRE_FALSE(generic_opt_float.has_value());
        REQUIRE_FALSE(generic_opt_double.has_value());
        REQUIRE_FALSE(generic_opt_guid.has_value());

        // do assign
        opt_bool   = true;
        opt_int16  = 1;
        opt_int32  = 2;
        opt_int64  = 3;
        opt_float  = 4.0f;
        opt_double = 5.0;
        opt_guid   = u8"051b41eb-2f26-4509-9032-0ca30d9b1990"_guid;

        // check assigned has value
        REQUIRE(generic_opt_bool.has_value());
        REQUIRE(generic_opt_int16.has_value());
        REQUIRE(generic_opt_int32.has_value());
        REQUIRE(generic_opt_int64.has_value());
        REQUIRE(generic_opt_float.has_value());
        REQUIRE(generic_opt_double.has_value());
        REQUIRE(generic_opt_guid.has_value());

        // generic reset
        generic_opt_bool.reset();
        generic_opt_int16.reset();
        generic_opt_int32.reset();
        generic_opt_int64.reset();
        generic_opt_float.reset();
        generic_opt_double.reset();
        generic_opt_guid.reset();

        // check reset has value
        REQUIRE_FALSE(generic_opt_bool.has_value());
        REQUIRE_FALSE(generic_opt_int16.has_value());
        REQUIRE_FALSE(generic_opt_int32.has_value());
        REQUIRE_FALSE(generic_opt_int64.has_value());
        REQUIRE_FALSE(generic_opt_float.has_value());
        REQUIRE_FALSE(generic_opt_double.has_value());
        REQUIRE_FALSE(generic_opt_guid.has_value());

        REQUIRE_FALSE(opt_bool.has_value());
        REQUIRE_FALSE(opt_int16.has_value());
        REQUIRE_FALSE(opt_int32.has_value());
        REQUIRE_FALSE(opt_int64.has_value());
        REQUIRE_FALSE(opt_float.has_value());
        REQUIRE_FALSE(opt_double.has_value());
        REQUIRE_FALSE(opt_guid.has_value());
    }
#endif
}