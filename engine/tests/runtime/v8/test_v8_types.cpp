#include "./test_v8_types.hpp"

namespace test_v8
{
int32_t  BasicObject::test_ctor_value      = 0;
int64_t  BasicObject::test_static_method_v = 0;
uint64_t BasicObject::test_static_field    = 0;

int32_t  BasicValue::test_ctor_value      = 0;
int64_t  BasicValue::test_static_method_v = 0;
uint64_t BasicValue::test_static_field    = 0;

BasicMapping   BasicMappingHelper::basic_value   = {};
InheritMapping BasicMappingHelper::inherit_value = {};

BasicEnum   BasicEnumHelper::test_value = BasicEnum::Value1;
skr::String BasicEnumHelper::test_name  = {};

skr::String ParamFlagTest::test_value = u8"";

skr::String TestString::value = {};

RttrMixin* MixinHelper::mixin = nullptr;

TestMixinValue MixinHelper::test_value = {};

} // namespace test_v8
