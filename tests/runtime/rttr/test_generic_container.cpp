#include "SkrRTTR/export/export_builder.hpp"
#include "SkrRTTR/generic/generic_vector.hpp"
#include "SkrTestFramework/framework.hpp"
#include "SkrCore/log.hpp"
#include "SkrRTTR/generic/generic_optional.hpp"
#include "SkrContainersDef/optional.hpp"
#include "SkrCore/exec_static.hpp"

struct OpTester {
    static int  ctor_count;
    static int  dtor_count;
    static int  copy_count;
    static int  move_count;
    static int  assign_count;
    static int  move_assign_count;
    static void reset_all_count()
    {
        ctor_count        = 0;
        dtor_count        = 0;
        copy_count        = 0;
        move_count        = 0;
        assign_count      = 0;
        move_assign_count = 0;
    }

    inline OpTester()
    {
        ++ctor_count;
    }
    inline ~OpTester()
    {
        ++dtor_count;
    }
    inline OpTester(const OpTester&)
    {
        ++copy_count;
    }
    inline OpTester(OpTester&&)
    {
        ++move_count;
    }
    inline OpTester& operator=(const OpTester&)
    {
        ++assign_count;
        return *this;
    }
    inline OpTester& operator=(OpTester&&)
    {
        ++move_assign_count;
        return *this;
    }
};

int OpTester::ctor_count        = 0;
int OpTester::dtor_count        = 0;
int OpTester::copy_count        = 0;
int OpTester::move_count        = 0;
int OpTester::assign_count      = 0;
int OpTester::move_assign_count = 0;

SKR_RTTR_TYPE(OpTester, "7b3e54f1-1d3a-49f6-b15b-bfe439e14076")
SKR_EXEC_STATIC_CTOR
{
    skr::register_type_loader(
        skr::type_id_of<OpTester>(),
        +[](skr::RTTRType* type) {
            type->build_record([](skr::RTTRRecordData* data) {
                skr::RTTRRecordBuilder<OpTester> builder(data);
                builder.basic_info();
            });
        }
    );
};

TEST_CASE("test generic optional")
{
    using namespace skr;

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

TEST_CASE("test generic vector")
{
    using namespace skr;

    RC<GenericVector> generic_vec = build_generic(type_signature_of<Vector<int32_t>>()).cast_static<GenericVector>();

    SUBCASE("data get")
    {
        Vector<int32_t> vec;

        // check data
        REQUIRE_EQ(generic_vec->capacity(&vec), vec.capacity());
        REQUIRE_EQ(generic_vec->slack(&vec), vec.slack());
        REQUIRE_EQ(generic_vec->size(&vec), vec.size());
        REQUIRE_EQ(generic_vec->is_empty(&vec), vec.is_empty());
        REQUIRE_EQ(generic_vec->data(&vec), vec.data());

        // check data with content
        vec = { 1, 1, 4, 5, 1, 4 };
        REQUIRE_EQ(generic_vec->capacity(&vec), vec.capacity());
        REQUIRE_EQ(generic_vec->slack(&vec), vec.slack());
        REQUIRE_EQ(generic_vec->size(&vec), vec.size());
        REQUIRE_EQ(generic_vec->is_empty(&vec), vec.is_empty());
        REQUIRE_EQ(generic_vec->data(&vec), vec.data());
    }

    SUBCASE("memory op")
    {
        Vector<int32_t> vec{ 1, 1, 4, 5, 1, 4 };

        // clear
        auto old_capacity = vec.capacity();
        generic_vec->clear(&vec);
        REQUIRE_EQ(vec.size(), 0);
        REQUIRE_EQ(vec.capacity(), old_capacity);
        REQUIRE_NE(vec.data(), nullptr);

        // release
        generic_vec->release(&vec);
        REQUIRE_EQ(vec.size(), 0);
        REQUIRE_EQ(vec.capacity(), 0);
        REQUIRE_EQ(vec.data(), nullptr);

        // release with reserve
        vec = { 1, 1, 4, 5, 1, 4 };
        generic_vec->release(&vec, 10);
        REQUIRE_EQ(vec.size(), 0);
        REQUIRE_EQ(vec.capacity(), 10);
        REQUIRE_NE(vec.data(), nullptr);

        // reserve
        generic_vec->reserve(&vec, 114514);
        REQUIRE_EQ(vec.size(), 0);
        REQUIRE_EQ(vec.capacity(), 114514);
        REQUIRE_NE(vec.data(), nullptr);

        // shrink
        generic_vec->shrink(&vec);
        REQUIRE_EQ(vec.size(), 0);
        REQUIRE_EQ(vec.capacity(), 0);
        REQUIRE_EQ(vec.data(), nullptr);

        // resize
        int32_t new_value = 114514;
        generic_vec->resize(&vec, 10, &new_value);
        REQUIRE_EQ(vec.size(), 10);
        REQUIRE_EQ(vec.capacity(), 10);
        for (size_t i = 0; i < vec.size(); ++i)
        {
            REQUIRE_EQ(vec[i], new_value);
        }

        // resize default
        generic_vec->resize_default(&vec, 5);
        REQUIRE_EQ(vec.size(), 5);
        REQUIRE_GE(vec.capacity(), 5);
        for (size_t i = 0; i < vec.size(); ++i)
        {
            REQUIRE_EQ(vec[i], 114514);
        }

        // resize zeroed
        generic_vec->resize_zeroed(&vec, 114514);
        REQUIRE_EQ(vec.size(), 114514);
        REQUIRE_GE(vec.capacity(), 114514);
        for (size_t i = 0; i < 5; ++i)
        {
            REQUIRE_EQ(vec[i], 114514);
        }
        for (size_t i = 5; i < vec.size(); ++i)
        {
            REQUIRE_EQ(vec[i], 0);
        }
    }

    SUBCASE("add")
    {
        
    }

    SUBCASE("add at")
    {
    }
}