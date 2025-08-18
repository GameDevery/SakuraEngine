#pragma once
#include "SkrBase/config.h"

#ifdef __cplusplus
    #include "./impl/md5.hpp"

using skr_md5_t = skr::MD5;

SKR_EXTERN_C bool skr_parse_md5(const char8_t* str32, skr_md5_t* out_md5);
SKR_EXTERN_C void skr_make_md5(const char8_t* str, uint32_t str_size, skr_md5_t* out_md5);

namespace skr
{
inline MD5 MD5::Parse(const char8_t* str)
{
    MD5 result;
    if (skr_parse_md5(str, &result))
    {
        return result;
    }
    else
    {
        return {};
    }
}
inline MD5 MD5::Make(const char8_t* str, uint32_t str_size)
{
    MD5 result;
    skr_make_md5(str, str_size, &result);
    return result;
}

} // namespace skr

#else
typedef struct skr_md5_t {
    uint8_t digest[128 / 8];
} skr_md5_t;

SKR_EXTERN_C bool skr_parse_md5(const char8_t* str32, skr_md5_t* out_md5);
SKR_EXTERN_C void skr_make_md5(const char8_t* str, uint32_t str_size, skr_md5_t* out_md5);
#endif