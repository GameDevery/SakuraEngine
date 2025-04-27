#pragma once
#include "SkrBase/config.h"

#define SKR_MD5_DIGEST_LENGTH 128 / 8
typedef struct skr_md5_t {
    uint8_t digest[SKR_MD5_DIGEST_LENGTH];
} skr_md5_t;

typedef struct skr_md5_u32x4_view_t {
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;
} skr_md5_u32x4_view_t;

#ifdef __cplusplus
namespace skr
{
using MD5 = skr_md5_t;
}
inline bool operator==(skr_md5_t a, skr_md5_t b)
{
    const skr_md5_u32x4_view_t* va     = (skr_md5_u32x4_view_t*)&a;
    const skr_md5_u32x4_view_t* vb     = (skr_md5_u32x4_view_t*)&b;
    int                         result = true;
    result &= (va->a == vb->a);
    result &= (va->b == vb->b);
    result &= (va->c == vb->c);
    result &= (va->d == vb->d);
    return result;
}
#endif

SKR_EXTERN_C bool skr_parse_md5(const char8_t* str32, skr_md5_t* out_md5);
SKR_EXTERN_C void skr_make_md5(const char8_t* str, uint32_t str_size, skr_md5_t* out_md5);