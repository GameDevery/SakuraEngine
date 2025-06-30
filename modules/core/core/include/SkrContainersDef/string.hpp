#pragma once
#include "SkrBase/config.h"
#include "SkrBase/types.h"
#include "SkrContainersDef/skr_allocator.hpp"
#include "SkrBase/misc/hash.h"
#include "SkrBase/containers/string/string_memory.hpp"
#include "SkrBase/containers/string/string.hpp"
#include "SkrBase/containers/string/format.hpp"

namespace skr
{
constexpr uint64_t kStringSSOSize = 31;

// string types
using String               = container::U8String<container::StringMemory<
              skr_char8,      /*type*/
              uint64_t,       /*size type*/
              kStringSSOSize, /*sso size*/
              SkrAllocator    /*allocator*/
              >>;
using StringView           = container::U8StringView<uint64_t>;
using SerializeConstString = String;

// format
template <typename... Args>
inline void format_to(String& out, StringView fmt, Args&&... args)
{
    container::format_to(out, fmt, std::forward<Args>(args)...);
}
template <typename... Args>
inline String format(StringView fmt, Args&&... args)
{
    return container::format<String>(fmt, std::forward<Args>(args)...);
}

SKR_STATIC_API bool guid_from_sv(const skr::StringView& str, skr_guid_t& value);

// hash
template <>
struct Hash<String> {
    inline skr_hash operator()(const String& x) const
    {
        return skr_hash_of(x.c_str(), x.size(), 0);
    }
    inline skr_hash operator()(const StringView& x) const
    {
        return skr_hash_of(x.data(), x.size(), 0);
    }
    inline skr_hash operator()(const char* x) const
    {
        return skr_hash_of(x, std::strlen(x), 0);
    }
    inline skr_hash operator()(const char8_t* x) const
    {
        return skr_hash_of(x, std::strlen(reinterpret_cast<const char*>(x)), 0);
    }
};
template <>
struct Hash<StringView> {
    inline skr_hash operator()(const StringView& x) const
    {
        return skr_hash_of(x.data(), x.size(), 0);
    }
};
} // namespace skr


// skr type integration
namespace skr::container
{
template <>
struct Formatter<skr_md5_t> {
    template <typename TString>
    inline static void format(TString& out, const skr_md5_t& md5, typename TString::ViewType spec)
    {
        ::skr::format_to(out,
                         u8"{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}",
                         md5.digest[0], md5.digest[1], md5.digest[2], md5.digest[3],
                         md5.digest[4], md5.digest[5], md5.digest[6], md5.digest[7],
                         md5.digest[8], md5.digest[9], md5.digest[10], md5.digest[11],
                         md5.digest[12], md5.digest[13], md5.digest[14], md5.digest[15]);
    }
};
template <>
struct Formatter<skr_guid_t> {
    template <typename TString>
    inline static void format(TString& out, const skr_guid_t& guid, typename TString::ViewType spec)
    {
        ::skr::format_to(out,
                         u8"{:08x}-{:04x}-{:04x}-{:02x}{:02x}-{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}",
                         guid.data1(), guid.data2(), guid.data3(),
                         guid.data4(0), guid.data4(1), guid.data4(2),
                         guid.data4(3), guid.data4(4), guid.data4(5),
                         guid.data4(6), guid.data4(7));
    }
};
} // namespace skr::container