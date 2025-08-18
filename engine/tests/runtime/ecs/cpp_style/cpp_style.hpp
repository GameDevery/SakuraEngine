#pragma once
#include "SkrRT/sugoi/sugoi.h"
#include "SkrRT/sugoi/array.hpp"
#include "SkrRT/sugoi/type_registry.hpp"
#include "SkrTestFramework/framework.hpp"
#ifndef __meta__
    #include "cpp_style.generated.h"
#endif

template <typename Result>
inline static void _EXPECT_OK(Result&& r)
{
    using namespace skr::archive;
    r.and_then([](auto type_index) {
        EXPECT_EQ(true, true);
    })
    .error_then([](auto error) {
        EXPECT_EQ(true, false);
    });
}

template <typename Result, typename ErrorCode>
inline static void _EXPECT_ERROR(Result&& r, ErrorCode err_code)
{
    using namespace skr::archive;
    r.and_then([](auto type_index) {
        EXPECT_EQ(true, false);
    })
    .error_then([=](auto error) {
        EXPECT_EQ(error, err_code);
    });
}

// for better ide display
#define EXPECT_OK _EXPECT_OK
#define EXPECT_ERROR _EXPECT_ERROR

sreflect_struct(
    guid = "12d2301d-b741-4191-acf6-d4fa7c7f64d5"
    ecs.comp = @enable
)
IntComponent {
    int v;
};

sreflect_struct(
    guid = "9bcb423f-aa5b-45dc-87b8-0f805686e777"
    ecs.comp = @enable
)
FloatComponent {
    float v;
};

sreflect_struct(
    guid = "a54016fa-a6b7-4224-87a7-cb1da8abe8ed"
    ecs.comp = @enable
)
SharedComponent {
    int i;
    float f;
};