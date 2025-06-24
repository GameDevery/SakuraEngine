#pragma once
#include "SkrBase/config.h"
#include "SkrBase/misc/hash.hpp"
#include "SkrBase/misc/debug.h"
#include <initializer_list>

namespace skr
{
struct GUID {
    // ctors
    inline constexpr GUID() = default;
    inline constexpr GUID(uint32_t b0, uint16_t b1, uint16_t b2, const uint8_t b3s[8])
        : storage0(b0)
        , storage1(_combine_16(b1, b2))
        , storage2(_combine_8(b3s[0], b3s[1], b3s[2], b3s[3]))
        , storage3(_combine_8(b3s[4], b3s[5], b3s[6], b3s[7]))
    {
    }
    inline constexpr GUID(uint32_t b0, uint16_t b1, uint16_t b2, std::initializer_list<uint8_t> b3s_l)
        : storage0(b0)
        , storage1(_combine_16(b1, b2))
        , storage2(_combine_8(b3s_l.begin()[0], b3s_l.begin()[1], b3s_l.begin()[2], b3s_l.begin()[3]))
        , storage3(_combine_8(b3s_l.begin()[4], b3s_l.begin()[5], b3s_l.begin()[6], b3s_l.begin()[7]))
    {
    }

    // getters
    inline constexpr bool     is_zero() const { return !(storage0 && storage1 && storage2 && storage3); }
    inline constexpr uint32_t data1() const { return storage0; }
    inline constexpr uint16_t data2() const { return ((uint16_t*)&storage1)[0]; }
    inline constexpr uint16_t data3() const { return ((uint16_t*)&storage1)[1]; }
    inline constexpr uint8_t  data4(uint8_t idx0_7) const { return ((uint8_t*)&storage2)[idx0_7]; }

    // factory
    static GUID Create();
    static GUID Parse(const char8_t* str);
    static GUID Parse(const char8_t* str, size_t len);

    // hash
    inline constexpr size_t get_hash() const
    {
        using namespace skr;
        constexpr Hash<uint64_t> hasher{};

        size_t result = hasher.operator()(static_cast<uint64_t>(storage0) << 32 | storage1);
        return hash_combine(result, hasher.operator()(static_cast<uint64_t>(storage2) << 32 | storage3));
    }

    // for skr::Hash
    inline static size_t _skr_hash(const GUID& guid)
    {
        return guid.get_hash();
    }

    // compare
    inline constexpr friend bool operator==(const GUID& a, const GUID& b)
    {
        int result = true;
        result &= (a.storage0 == b.storage0);
        result &= (a.storage0 == b.storage0);
        result &= (a.storage0 == b.storage0);
        result &= (a.storage0 == b.storage0);
        return result;
    }
    inline constexpr friend bool operator!=(const GUID& a, const GUID& b)
    {
        return !(a == b);
    }
    inline constexpr friend bool operator<(const GUID& a, const GUID& b)
    {
        if (a.storage0 != b.storage0) return a.storage0 < b.storage0;
        if (a.storage1 != b.storage1) return a.storage1 < b.storage1;
        if (a.storage2 != b.storage2) return a.storage2 < b.storage2;
        return a.storage3 < b.storage3;
    }
    inline constexpr friend bool operator>(const GUID& a, const GUID& b)
    {
        if (a.storage0 != b.storage0) return a.storage0 > b.storage0;
        if (a.storage1 != b.storage1) return a.storage1 > b.storage1;
        if (a.storage2 != b.storage2) return a.storage2 > b.storage2;
        return a.storage3 > b.storage3;
    }
    inline constexpr friend bool operator<=(const GUID& a, const GUID& b)
    {
        return !(a > b);
    }
    inline constexpr friend bool operator>=(const GUID& a, const GUID& b)
    {
        return !(a < b);
    }

private:
    inline constexpr uint32_t _combine_16(uint16_t a, uint16_t b) const
    {
#if SKR_LITTLE_ENDIAN
        return a | b << 16;
#else
        return a << 16 | b;
#endif
    }
    inline constexpr uint32_t _combine_8(uint8_t a, uint8_t b, uint8_t c, uint8_t d) const
    {
#if SKR_LITTLE_ENDIAN
        return a | b << 8 | c << 16 | d << 24;
#else
        return a << 24 | b << 16 | c << 8 | d;
#endif
    }

public:
    uint32_t storage0 = 0;
    uint32_t storage1 = 0;
    uint32_t storage2 = 0;
    uint32_t storage3 = 0;
};

namespace details
{

constexpr const size_t short_guid_form_length = 36; // XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX
constexpr const size_t long_guid_form_length  = 38; // {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}

constexpr int parse_hex_digit(const char8_t c)
{
    if ('0' <= c && c <= '9')
        return c - '0';
    else if ('a' <= c && c <= 'f')
        return 10 + c - 'a';
    else if ('A' <= c && c <= 'F')
        return 10 + c - 'A';
    else
        SKR_ASSERT(0 && "invalid character in GUID");
    return -1;
}

template <class T>
constexpr T parse_hex(const char8_t* ptr)
{
    constexpr size_t digits = sizeof(T) * 2;
    T                result{};
    for (size_t i = 0; i < digits; ++i)
        result |= parse_hex_digit(ptr[i]) << (4 * (digits - i - 1));
    return result;
}

constexpr GUID make_guid_helper(const char8_t* begin)
{
    auto Data1 = parse_hex<uint32_t>(begin);
    begin += 8 + 1;
    auto Data2 = parse_hex<uint16_t>(begin);
    begin += 4 + 1;
    auto Data3 = parse_hex<uint16_t>(begin);
    begin += 4 + 1;
    uint8_t Data4[8] = {};
    Data4[0]         = parse_hex<uint8_t>(begin);
    begin += 2;
    Data4[1] = parse_hex<uint8_t>(begin);
    begin += 2 + 1;
    for (size_t i = 0; i < 6; ++i)
        Data4[i + 2] = parse_hex<uint8_t>(begin + i * 2);
    return GUID(Data1, Data2, Data3, Data4);
}
} // namespace details

inline namespace literals
{
constexpr GUID operator""_guid(const char8_t* str, size_t N)
{
    using namespace ::skr::details;
    if (!(N == long_guid_form_length || N == short_guid_form_length))
        SKR_ASSERT(0 && "String GUID of the form {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX} or XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX is expected");
    if (N == long_guid_form_length && (str[0] != '{' || str[long_guid_form_length - 1] != '}'))
        SKR_ASSERT(0 && "Missing opening or closing brace");

    return make_guid_helper(str + (N == long_guid_form_length ? 1 : 0));
}
} // namespace literals

inline GUID GUID::Parse(const char8_t* str)
{
    return Parse(str, std::char_traits<char8_t>::length(str));
}
inline GUID GUID::Parse(const char8_t* str, size_t len)
{
    using namespace ::skr::details;
    SKR_ASSERT(len == long_guid_form_length || len == short_guid_form_length);
    return make_guid_helper(str + (len == long_guid_form_length ? 1 : 0));
}

} // namespace skr