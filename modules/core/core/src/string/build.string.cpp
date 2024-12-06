#include "OpenString/platforms.cpp"
#include "OpenString/codeunit_sequence.cpp"
#include "OpenString/text.cpp"
#include "OpenString/wide_text.cpp"
#include "OpenString/format.cpp"
#include "SkrCore/log.h"
#include "SkrContainersDef/string.hpp"

namespace skr
{

constexpr int parse_hex_digit(const char8_t c)
{
    if ('0' <= c && c <= '9')
        return c - '0';
    else if ('a' <= c && c <= 'f')
        return 10 + c - 'a';
    else if ('A' <= c && c <= 'F')
        return 10 + c - 'A';
    else
        SKR_LOG_ERROR(u8"Invalid character in GUID. Expected hex digit, got %c", c);
    return -1;
}

template <class T>
bool parse_hex(const char8_t* ptr, T& value)
{
    constexpr size_t digits = sizeof(T) * 2;
    value                   = 0;
    for (size_t i = 0; i < digits; ++i)
    {
        int result = parse_hex_digit(ptr[i]);
        if (result < 0)
            return false;
        value |= result << (4 * (digits - i - 1));
    }
    return true;
}

bool make_guid_helper(const char8_t* begin, skr_guid_t& value)
{
    uint32_t Data1    = 0;
    uint16_t Data2    = 0;
    uint16_t Data3    = 0;
    uint8_t  Data4[8] = {};
    if (!parse_hex(begin, Data1))
        return false;
    begin += 8 + 1;
    if (!parse_hex(begin, Data2))
        return false;
    begin += 4 + 1;
    if (!parse_hex(begin, Data3))
        return false;
    begin += 4 + 1;
    if (!parse_hex(begin, Data4[0]))
        return false;
    begin += 2;
    if (!parse_hex(begin, Data4[1]))
        return false;
    begin += 2 + 1;
    for (size_t i = 0; i < 6; ++i)
        if (!parse_hex(begin + i * 2, Data4[i + 2]))
            return false;
    value = skr_guid_t(Data1, Data2, Data3, Data4);
    return true;
}

bool guid_from_sv(const skr::StringView& str, skr_guid_t& value)
{
    constexpr size_t short_guid_form_length = 36;
    constexpr size_t long_guid_form_length  = 38;

    if (str.size() != long_guid_form_length && str.size() != short_guid_form_length)
    {
        skr::String str2(str);
        SKR_LOG_ERROR(u8"String GUID of the form {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX} or XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX is expected, got %s", str2.c_str());
        return false;
    }

    if (str.size() == (long_guid_form_length + 1))
    {
        if (str.at_buffer(0) != u8'{' || str.at_buffer(long_guid_form_length - 1) != u8'}')
        {
            skr::String str2(str);
            SKR_LOG_ERROR(u8"Opening or closing brace is expected, got %s", str2.c_str());
            return false;
        }
    }

    return make_guid_helper(str.data() + (str.size() == (long_guid_form_length + 1) ? 1 : 0), value);
}

} // namespace skr