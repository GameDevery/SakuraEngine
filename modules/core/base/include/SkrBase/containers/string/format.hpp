#pragma once
#include <SkrBase/containers/string/string.hpp>
#include <charconv>
#include <SkrBase/misc/debug.h>
#include <cmath>

// format parser
namespace skr::container
{
template <typename TS>
struct FormatToken {
    using ViewType = U8StringView<TS>;

    enum class Kind
    {
        Unknow,       // bad token
        PlainText,    // normal text
        EscapedBrace, // {{ or }}
        Formatter,    // content inside {}
    };

    Kind     kind = Kind::Unknow;
    ViewType view = {};
};
template <typename TS>
struct FormatTokenIter {
    using ViewType  = U8StringView<TS>;
    using TokenType = FormatToken<TS>;

    inline FormatTokenIter(ViewType format_str)
        : _format_str(format_str)
        , _index(0)
        , _cur_token{}
    {
        if (!_format_str.is_empty())
        {
            _parse_token();
        }
    }

    inline TokenType ref()
    {
        return _cur_token;
    }
    inline void reset()
    {
        _index = 0;
        if (!_format_str.is_empty())
        {
            _parse_token();
        }
    }
    inline void move_next()
    {
        if (_index < _format_str.size())
        {
            _index += _cur_token.view.size();
            if (_index < _format_str.size())
            {
                _parse_token();
            }
        }
    }
    inline bool has_next()
    {
        return _index < _format_str.size();
    }

private:
    inline void _parse_token()
    {
        auto begin_ch = _format_str.at_buffer(_index);

        if (begin_ch == u8'{') // formatter or plain text
        {
            if (_index < _format_str.size() - 1 && _format_str.at_buffer(_index + 1) == u8'{')
            { // escaped brace
                _cur_token.kind = TokenType::Kind::EscapedBrace;
                _cur_token.view = _format_str.subview(_index, 2);
            }
            else
            { // formatted
                if (auto found = _format_str.subview(_index).find(UTF8Seq{ u8'}' }))
                {
                    _cur_token.kind = TokenType::Kind::Formatter;
                    _cur_token.view = _format_str.subview(_index, found.index() + 1);
                }
                else
                {
                    _cur_token.kind = TokenType::Kind::Unknow;
                    _cur_token.view = _format_str.subview(_index, 1);
                    SKR_VERIFY(false && "unclosed right brace is not allowed!");
                }
            }
        }
        else if (begin_ch == u8'}')
        {
            if (_index < _format_str.size() - 1 && _format_str.at_buffer(_index + 1) == u8'}')
            {
                _cur_token.kind = TokenType::Kind::EscapedBrace;
                _cur_token.view = _format_str.subview(_index, 2);
            }
            else
            {
                _cur_token.kind = TokenType::Kind::Unknow;
                _cur_token.view = _format_str.subview(_index, 1);
                SKR_VERIFY(false && "unclosed left brace is not allowed!");
            }
        }
        else
        {
            auto search_str        = _format_str.subview(_index);
            auto found_left_brace  = search_str.find(UTF8Seq{ u8'{' });
            auto found_right_brace = search_str.find(UTF8Seq{ u8'}' });
            if (found_left_brace || found_right_brace)
            {
                auto next_index = found_left_brace && found_right_brace ? std::min(found_left_brace.index(), found_right_brace.index()) :
                                  found_left_brace                      ? found_left_brace.index() :
                                                                          found_right_brace.index();
                _cur_token.kind = TokenType::Kind::PlainText;
                _cur_token.view = _format_str.subview(_index, next_index);
            }
            else
            {
                _cur_token.kind = TokenType::Kind::PlainText;
                _cur_token.view = _format_str.subview(_index);
            }
        }
    }

private:
    ViewType  _format_str;
    TS        _index;
    TokenType _cur_token;
};
} // namespace skr::container

// formatter
namespace skr::container
{
inline uint64_t get_integer_digit_count(int64_t value, int64_t base)
{
    if (value == 0)
        return 1;
    int64_t  remaining = value;
    uint64_t count     = 0;
    while (remaining != 0)
    {
        remaining /= base;
        ++count;
    }
    return count;
}
inline uint64_t get_integer_digit_count(uint64_t value, uint64_t base)
{
    if (value == 0)
        return 1;
    int64_t  remaining = value;
    uint64_t count     = 0;
    while (remaining != 0)
    {
        remaining /= base;
        ++count;
    }
    return count;
}
inline skr_char8 from_digit(uint64_t digit)
{
    constexpr skr_char8 digits[] = u8"0123456789abcdef";
    return digits[digit];
}
template <typename TString>
inline void format_integer(TString& out, uint64_t value, typename TString::ViewType spec)
{
    using ViewType = typename TString::ViewType;

    ViewType type_spec_view = u8"bcdox";

    skr_char8 type        = u8'd';
    skr_char8 holder      = u8'0';
    uint64_t  holding     = npos_of<uint64_t>;
    bool      with_prefix = false;

    if (!spec.is_empty())
    {
        ViewType parsing = spec;

        // parse type
        if (type_spec_view.contains(parsing.last_buffer(0)))
        {
            type    = parsing.last_buffer(0);
            parsing = parsing.subview(0, parsing.size() - 1);
        }

        // parse holder
        if (!parsing.is_empty())
        {
            if (auto found = parsing.find(u8'0'))
            {
                holder = found.ref();

                ViewType holding_view  = parsing.subview(found.index() + 1);
                const auto [last, err] = std::from_chars((const char*)holding_view.data(),
                                                         (const char*)holding_view.data() + holding_view.size(), holding);
                SKR_VERIFY((last == (const char*)(holding_view.data() + holding_view.size())) && "Invalid format specification!");
                parsing = parsing.subview(0, found.index());
            }
        }

        // parse prefix
        if (!parsing.is_empty())
        {
            if (parsing.last_buffer(0) == '#')
            {
                with_prefix = true;
                parsing     = parsing.subview(0, parsing.size() - 1);
            }
        }
        SKR_VERIFY(parsing.is_empty() && "Invalid format specification!");
    }

    // solve type
    uint64_t base = 10;
    ViewType prefix;
    switch (type)
    {
        case 'b': {
            base   = 2;
            prefix = u8"0b";
        }
        break;
        case 'c': {
            out.append(static_cast<skr_char8>(value));
            return;
        }
        case 'd':
            break;
        case 'o': {
            base   = 8;
            prefix = u8"0o";
        }
        break;
        case 'x': {
            base   = 16;
            prefix = u8"0x";
        }
        break;
        default:
            break;
    }
    if (!with_prefix)
        prefix = u8"";

    // final solve
    const uint64_t digit_count  = get_integer_digit_count(value, base);
    const uint64_t preserve     = holding == npos_of<uint64_t> ? digit_count : std::max(holding, digit_count);
    const uint64_t holder_count = preserve - digit_count;

    // append
    out.append(prefix);
    out.add(holder, holder_count);
    uint64_t remaining     = value;
    uint64_t reverse_start = out.size();
    for (uint64_t i = 0; i < digit_count; ++i)
    {
        const skr_char8 digit = from_digit(remaining % base);
        remaining /= base;
        out.append(digit);
    }
    out.reverse(reverse_start);
}
template <typename TString>
inline void format_integer(TString& out, int64_t value, typename TString::ViewType spec)
{
    using ViewType = typename TString::ViewType;

    ViewType type_spec_view = u8"bcdox";
    uint64_t  holding     = npos_of<uint64_t>;
    skr_char8 type        = u8'd';
    bool with_prefix = false; // #

    // // tail specs
    // bool binary  = false; // b
    // bool as_char = false; // c
    // bool decimal = false; // d
    // bool octal   = false; // 0
    // bool hex     = false; // x
    
    // // prifix specs
    // bool with_prefix            = false; // #
    // bool with_non_negative_sign = false; // +
    // bool with_prefix_zero       = false; // 0 just after [#+-]
    
    // // align
    // char align_fill_ch = u8' '; // ch befor [<>^]
    // bool left_align   = false; // <
    // bool right_align  = false; // >
    // bool center_align = false; // ^

    // // width & precision
    // uint64_t width = 0; // 0

    if (!spec.is_empty())
    {
        ViewType parsing = spec;

        // parse type
        if (type_spec_view.contains(parsing.last_buffer(0)))
        {
            type    = parsing.last_buffer(0);
            parsing = parsing.subview(0, parsing.size() - 1);
        }

        // parse holder
        if (!parsing.is_empty())
        {
            if (auto found = parsing.find(u8'0'))
            {
                ViewType holding_view  = parsing.subview(found.index() + 1);
                const auto [last, err] = std::from_chars((const char*)holding_view.data(),
                                                         (const char*)holding_view.data() + holding_view.size(), holding);
                SKR_VERIFY((last == (const char*)(holding_view.data() + holding_view.size())) && "Invalid format specification!");
                parsing = parsing.subview(0, found.index());
            }
        }

        // parse prefix
        if (!parsing.is_empty())
        {
            if (parsing.last_buffer(0) == '#')
            {
                with_prefix = true;
                parsing     = parsing.subview(0, parsing.size() - 1);
            }
        }
        SKR_VERIFY(parsing.is_empty() && "Invalid format specification!");
    }

    // solve type
    int64_t  base = 10;
    ViewType prefix;
    switch (type)
    {
        case 'b': {
            base   = 2;
            prefix = u8"0b";
        }
        break;
        case 'c': {
            out.append(static_cast<skr_char8>(value));
            return;
        }
        case 'd':
            break;
        case 'o': {
            base   = 8;
            prefix = u8"0o";
        }
        break;
        case 'x': {
            base   = 16;
            prefix = u8"0x";
        }
        break;
        default:
            break;
    }
    if (!with_prefix)
        prefix = u8"";

    // final solve
    const ViewType sign        = (value < 0) ? u8"-" : u8"";
    const uint64_t digit_count = get_integer_digit_count(value, base);
    const uint64_t preserve    = holding == npos_of<uint64_t> ? digit_count : std::max(holding, digit_count);
    const uint64_t zero_count  = preserve - digit_count;

    // append
    out.append(sign);
    out.append(prefix);
    out.add(u8'0', zero_count);
    int64_t  remaining     = value >= 0 ? value : -value;
    uint64_t reverse_start = out.size();
    for (uint64_t i = 0; i < digit_count; ++i)
    {
        skr_char8 digit = from_digit(remaining % base);
        remaining /= base;
        out.append(digit);
    }
    out.reverse(reverse_start);
}
template <typename TString>
inline void format_float(TString& out, double value, typename TString::ViewType spec)
{
    using ViewType = typename TString::ViewType;

    // inf & nan
    if (std::isinf(value))
    {
        out.append(value < 0 ? u8"-inf" : u8"inf");
        return;
    }
    if (std::isnan(value))
    {
        out.append(u8"nan");
        return;
    }

    // specs
    bool hex = false;         // a
    bool scientific = false;  // e
    bool fixed = false;       // f
    bool general = false;     // g
    uint64_t precision = 0;   // .xxx

    // solve spec
    if (!spec.is_empty())
    {
        // parse formats
        ViewType parsing = spec;
        switch (parsing.last_buffer(0))
        {
            case u8'a':
                hex = true;
                parsing = parsing.subview(0, parsing.size() - 1);
                break;
            case u8'e':
                scientific = true;
                parsing = parsing.subview(0, parsing.size() - 1);
                break;
            case u8'f':
                fixed = true;
                parsing = parsing.subview(0, parsing.size() - 1);
                break;
            case u8'g':
                general = true;
                parsing = parsing.subview(0, parsing.size() - 1);
                break;
            default:
                break;
        
        }

        // parse precision
        if (!parsing.is_empty())
        {
            if (auto found = parsing.find(u8'.'))
            {
                ViewType precision_view  = parsing.subview(found.index() + 1);
                const auto [last, error] = std::from_chars(
                    (const char*)precision_view.data(),
                    (const char*)(precision_view.data() + precision_view.size()),
                    precision
                );
                SKR_VERIFY((last == (const char*)(precision_view.data() + precision_view.size())) && "Invalid format specification!");
                parsing = parsing.subview(0, found.index());
            }
        }
        SKR_VERIFY(parsing.is_empty() && "Invalid format specification!");
    }

    // convert to flags
    std::chars_format format;
    if (hex)
    {
        format = std::chars_format::hex;
    }
    else if (scientific)
    {
        format = std::chars_format::scientific;
    }
    else if (fixed)
    {
        format = std::chars_format::fixed;
    }
    else if (general)
    {
        format = std::chars_format::general;
    }
    bool has_format = hex || scientific || fixed || general;
    
    // solve
    // see https://en.cppreference.com/w/cpp/utility/format/spec
    constexpr uint32_t buffer_count = 128;
    char buffer[buffer_count];
    std::to_chars_result result;
    if (has_format)
    {
        result = std::to_chars(
            buffer,
            buffer + buffer_count,
            value,
            format,
            precision
        );
    }
    else 
    {
        if (precision > 0)
        {
            result = std::to_chars(
                buffer,
                buffer + buffer_count,
                value,
                std::chars_format::general,
                precision
            );
        }
        else
        {
            result = std::to_chars(
                buffer,
                buffer + buffer_count,
                value
            );
        }
    }
    

    // append
    if (result.ec != std::errc())
    {
        SKR_VERIFY(false && "failed to convert float to string!");
    }
    else
    {
        auto size = result.ptr - buffer;
        if (size > 0)
        {
            out.append(buffer, size);
        }
    }
}

template <typename T>
struct Formatter {
    Formatter() = delete;
    // template <typename TString>
    // static void format(TString& out, const T& value, typename TString::ViewType spec);
};

// primitive types
template <std::signed_integral T>
struct Formatter<T> {
    template <typename TString>
    inline static void format(TString& out, T value, typename TString::ViewType spec)
    {
        format_integer(out, static_cast<int64_t>(value), spec);
    }
};
template <std::unsigned_integral T>
struct Formatter<T> {
    template <typename TString>
    inline static void format(TString& out, T value, typename TString::ViewType spec)
    {
        format_integer(out, static_cast<uint64_t>(value), spec);
    }
};
template <>
struct Formatter<float> {
    template <typename TString>
    inline static void format(TString& out, float value, typename TString::ViewType spec)
    {
        format_float(out, static_cast<double>(value), spec);
    }
};
template <>
struct Formatter<double> {
    template <typename TString>
    inline static void format(TString& out, double value, typename TString::ViewType spec)
    {
        format_float(out, value, spec);
    }
};
template <>
struct Formatter<bool> {
    template <typename TString>
    inline static void format(TString& out, bool value, typename TString::ViewType spec)
    {
        out.append(value ? u8"true" : u8"false");
    }
};

// raw string
template <>
struct Formatter<char> {
    template <typename TString>
    inline static void format(TString& out, char value, typename TString::ViewType spec)
    {
        out.append(value);
    }
};
template <>
struct Formatter<const char*> {
    template <typename TString>
    inline static void format(TString& out, const char* value, typename TString::ViewType spec)
    {
        out.append(value);
    }
};
template <size_t N>
struct Formatter<char[N]> {
    template <typename TString>
    inline static void format(TString& out, const char (&value)[N], typename TString::ViewType spec)
    {
        out.append(value);
    }
};
template <>
struct Formatter<skr_char8> {
    template <typename TString>
    inline static void format(TString& out, skr_char8 value, typename TString::ViewType spec)
    {
        out.append(value);
    }
};
template <>
struct Formatter<const skr_char8*> {
    template <typename TString>
    inline static void format(TString& out, const skr_char8* value, typename TString::ViewType spec)
    {
        out.append(value);
    }
};
template <size_t N>
struct Formatter<skr_char8[N]> {
    template <typename TString>
    inline static void format(TString& out, const skr_char8 (&value)[N], typename TString::ViewType spec)
    {
        out.append(value);
    }
};

// string types
template <typename TS>
struct Formatter<U8StringView<TS>> {
    template <typename TString>
    inline static void format(TString& out, U8StringView<TS> value, typename TString::ViewType spec)
    {
        out.append(value);
    }
};
template <typename Memory>
struct Formatter<U8String<Memory>> {
    template <typename TString>
    inline static void format(TString& out, const U8String<Memory>& value, typename TString::ViewType spec)
    {
        out.append(value);
    }
};

// stl string types
template <>
struct Formatter<std::string_view> {
    template <typename TString>
    inline static void format(TString& out, std::string_view value, typename TString::ViewType spec)
    {
        out.append(value.data(), value.size());
    }
};
template <>
struct Formatter<std::string> {
    template <typename TString>
    inline static void format(TString& out, const std::string& value, typename TString::ViewType spec)
    {
        out.append(value.data(), value.size());
    }
};
template <>
struct Formatter<std::u8string_view> {
    template <typename TString>
    inline static void format(TString& out, std::u8string_view value, typename TString::ViewType spec)
    {
        out.append(value.data(), value.size());
    }
};
template <>
struct Formatter<std::u8string> {
    template <typename TString>
    inline static void format(TString& out, const std::u8string& value, typename TString::ViewType spec)
    {
        out.append(value.data(), value.size());
    }
};

// pointer
template <>
struct Formatter<std::nullptr_t> {
    template <typename TString>
    inline static void format(TString& out, std::nullptr_t value, typename TString::ViewType spec)
    {
        out.append(u8"nullptr");
    }
};
template <>
struct Formatter<void*> {
    template <typename TString>
    inline static void format(TString& out, const void* value, typename TString::ViewType spec)
    {
        format_integer(out, reinterpret_cast<uint64_t>(value), u8"#016x");
    }
};
template <>
struct Formatter<const void*> {
    template <typename TString>
    inline static void format(TString& out, const void* value, typename TString::ViewType spec)
    {
        format_integer(out, reinterpret_cast<uint64_t>(value), u8"#016x");
    }
};
} // namespace skr::container

// format
namespace skr::container
{
template <typename TString, typename T>
concept Formattable = requires(TString& out, const T& v, typename TString::ViewType spec) {
    Formatter<T>::format(out, v, spec);
};

template <typename TString>
struct ArgFormatterPack {
    using FormatterFunc = void (*)(TString&, const void*, typename TString::ViewType);

    template <typename T>
    inline ArgFormatterPack(const T& v)
        : arg(reinterpret_cast<const void*>(&v))
        , formatter(_make_formatter<T>())
    {
        static_assert(Formattable<TString, T>, "Type is not formattable!");
    }

    inline void format(TString& out, typename TString::ViewType& spec)
    {
        formatter(out, arg, spec);
    }

private:
    template <typename T>
    inline FormatterFunc _make_formatter()
    {
        return +[](TString& out, const void* value, typename TString::ViewType spec) {
            Formatter<T>::format(out, *reinterpret_cast<const T*>(value), spec);
        };
    }

public:
    const void*   arg;
    FormatterFunc formatter;
};

template <typename TString, typename... Args>
inline void format_to(TString& out, typename TString::ViewType view, Args&&... args)
{
    using SizeType  = typename TString::SizeType;
    using TokenIter = FormatTokenIter<SizeType>;

    constexpr uint64_t                               arg_count = sizeof...(Args);
    std::array<ArgFormatterPack<TString>, arg_count> formatters{ { ArgFormatterPack<TString>{ std::forward<Args>(args) }... } };

    out.reserve(out.size() + view.size());

    enum class IndexingMode
    {
        Unknown,
        Auto,
        Manual
    };
    IndexingMode indexing_mode = IndexingMode::Unknown;
    uint64_t     auto_index    = 0;
    for (TokenIter iter{ view }; iter.has_next(); iter.move_next())
    {
        auto token = iter.ref();
        switch (token.kind)
        {
            case FormatToken<SizeType>::Kind::PlainText:
                out.append(token.view);
                break;
            case FormatToken<SizeType>::Kind::EscapedBrace:
                out.append(token.view.first(1));
                break;
            case FormatToken<SizeType>::Kind::Formatter: {
                auto [arg_idx, mid, spec] = token.view.subview(1, token.view.size() - 2).partition(UTF8Seq{ u8':' });
                uint64_t cur_idx          = auto_index;
                if (arg_idx.is_empty())
                {
                    SKR_VERIFY(indexing_mode != IndexingMode::Manual && "Manual index is not allowed mixing with automatic index!");
                    indexing_mode = IndexingMode::Auto;
                    ++auto_index;
                }
                else
                {
                    SKR_VERIFY(indexing_mode != IndexingMode::Auto && "Automatic index is not allowed mixing with manual index!");
                    indexing_mode      = IndexingMode::Manual;
                    auto [last, error] = std::from_chars((const char*)arg_idx.data(),
                                                         (const char*)(arg_idx.data() + arg_idx.size()), cur_idx);
                    SKR_VERIFY(last == (const char*)(arg_idx.data() + arg_idx.size()) && "Invalid format index!");
                }
                SKR_VERIFY(cur_idx < arg_count && "Invalid format index!");
                formatters.at(cur_idx).format(out, spec);
            }
            break;
            default:
                break;
        }
    }
}

template <typename TString, typename... Args>
inline TString format(typename TString::ViewType view, Args&&... args)
{
    TString out;
    format_to(out, view, std::forward<Args>(args)...);
    return out;
}

// TODO. format, 返回 string
// TODO. format_to, 对特定容器进行 format
// TODO. formatted_size, format 后的尺寸
// TODO. vformat, 使用 FormatArgs 结构
} // namespace skr::container