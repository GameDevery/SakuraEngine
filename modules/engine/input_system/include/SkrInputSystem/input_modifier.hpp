#pragma once
#include "SkrInputSystem/input_value.hpp"

namespace skr {
namespace input {

struct SKR_INPUT_SYSTEM_API InputModifier : public RC
{
    virtual InputValueStorage modify_raw(const InputValueStorage& raw) SKR_NOEXCEPT = 0;
};

struct Shuffle
{
    int32_t shuffle[4] = { 0, 1, 2, 3 };
};

struct SKR_INPUT_SYSTEM_API InputModifierShuffle : public InputModifier
{
    InputValueStorage modify_raw(const InputValueStorage& raw) SKR_NOEXCEPT final;
    Shuffle shuffle;
};

struct SKR_INPUT_SYSTEM_API InputModifierScale : public InputModifier
{
    InputValueStorage modify_raw(const InputValueStorage& raw) SKR_NOEXCEPT final;
    skr_float4_t scale = { 1.f, 1.f, 1.f, 1.f };
};

} }