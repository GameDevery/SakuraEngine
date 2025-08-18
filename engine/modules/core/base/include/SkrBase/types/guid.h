#pragma once
#ifdef __cplusplus
    #include "./impl/guid.hpp" // IWYU pragma: export

using skr_guid_t = ::skr::GUID;

SKR_EXTERN_C void skr_make_guid(skr_guid_t* out_guid);
namespace skr
{
inline GUID GUID::Create()
{
    GUID guid;
    skr_make_guid(&guid);
    return guid;
}
} // namespace skr
#else
typedef struct skr_guid_t {
    uint32_t storage0;
    uint32_t storage1;
    uint32_t storage2;
    uint32_t storage3;
} skr_guid_t;
SKR_EXTERN_C void skr_make_guid(skr_guid_t* out_guid);
#endif