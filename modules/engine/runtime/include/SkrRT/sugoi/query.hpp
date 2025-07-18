#pragma once
#include "SkrRT/sugoi/sugoi.h"

struct sugoi_query_t
{
    struct Impl;
    Impl* pimpl = nullptr;
};