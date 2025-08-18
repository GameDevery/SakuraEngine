#pragma once
#include "SkrBase/misc/debug.h"

#ifdef __cplusplus
    #include "SkrBase/misc/bit.hpp"
    #include "SkrBase/misc/integer_tools.hpp"
#endif

// debug break marcos
#define SKR_DEBUG_BREAK() debug_break()

// tool marcos
#define SKR_DELETE_COPY(__TYPE)                \
    __TYPE(const __TYPE&)            = delete; \
    __TYPE& operator=(const __TYPE&) = delete;

#define SKR_DELETE_MOVE(__TYPE)           \
    __TYPE(__TYPE&&)            = delete; \
    __TYPE& operator=(__TYPE&&) = delete;

#define SKR_DELETE_COPY_MOVE(__TYPE) \
    SKR_DELETE_COPY(__TYPE)          \
    SKR_DELETE_MOVE(__TYPE)