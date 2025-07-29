#pragma once
// platform headers
#ifndef __cplusplus
    #include <stdbool.h>
#endif

#if __has_include("sys/types.h")
    #include <sys/types.h>
#endif

#if __has_include("stdint.h")
    #include <stdint.h>
#endif

#if __has_include("float.h")
    #include <float.h>
#endif

// platform & compiler marcos
#include "SkrBase/config/platform.h" // IWYU pragma: export
#include "SkrBase/config/compiler.h" // IWYU pragma: export

// keywords
#include "SkrBase/config/key_words.h" // IWYU pragma: export
#include "SkrBase/config/values.h" // IWYU pragma: export

// character
#include "SkrBase/config/character.h" // IWYU pragma: export