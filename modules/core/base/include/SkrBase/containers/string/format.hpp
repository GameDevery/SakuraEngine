#pragma once
#include <SkrBase/containers/string/string.hpp>
#include <format>

namespace skr::help
{
template <typename TS>
struct FormatToken {
    using ViewType = container::U8StringView<TS>;

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
    using ViewType  = container::U8StringView<TS>;
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
                    _cur_token.view = _format_str.subview(_index, found.index() - _index + 1);
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
            auto found_left_brace  = _format_str.find(UTF8Seq{ u8'{' });
            auto found_right_brace = _format_str.find(UTF8Seq{ u8'}' });
            if (found_left_brace || found_right_brace)
            {
                auto next_index = found_left_brace && found_right_brace ? std::min(found_left_brace.index(), found_right_brace.index()) :
                                  found_left_brace                      ? found_left_brace.index() :
                                                                          found_right_brace.index();
                _cur_token.kind = TokenType::Kind::PlainText;
                _cur_token.view = _format_str.subview(_index, next_index - _index);
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
} // namespace skr::help

namespace skr
{
template <typename T>
concept StdFormattable = requires(::std::formatter<T> formatter) {
    formatter.parse(std::basic_format_parse_context<skr_char8>{ {} });
};

template <typename T>
struct Formatter;

template <StdFormattable T>
struct Formatter<T> {
};

// TODO. format, 返回 string
// TODO. format_to, 对特定容器进行 format
// TODO. formatted_size, format 后的尺寸
// TODO. vformat, 使用 FormatArgs 结构
} // namespace skr