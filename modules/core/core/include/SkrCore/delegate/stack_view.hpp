#pragma once
#include "SkrRTTR/type_signature.hpp"

// TODO. StackView 用于脚本对 delegate/event 的兼容
namespace skr
{
struct StackView {
    struct Param {
        TypeSignatureView type;
        void*                   data;
    };
};
} // namespace skr