#pragma once
#include "../internal/common.hxx"

using int16 = short;
using uint16 = unsigned short;
using int32 = int;
using int64 = long long;
using uint32 = unsigned int;
using uint = uint32;
using uint64 = unsigned long long;

struct [[builtin("half")]] half {
    [[ignore]] half() = default;
    [[ignore]] half(float);
    [[ignore]] half(uint32);
    [[ignore]] half(int32);
private:
    short v;
};