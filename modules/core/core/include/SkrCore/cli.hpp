#pragma once
#include "SkrBase/config.h"
#include "SkrBase/meta.h"
#include "SkrContainersDef/string.hpp"
#include "SkrRTTR/generic_container.hpp"
#include "SkrRTTR/type.hpp"
#ifndef __meta__
    #include "SkrCore/SkrCore/cli.generated.h"
#endif

// attributes
namespace skr::attr
{
// clang-format off
sreflect_struct(guid = "5ec06e6b-ca62-45ca-aa35-d39e1aba88a4")
CmdOption {
    // clang-format on

    String    name        = {};
    skr_char8 short_name  = {};
    String    help        = {};
    bool      is_required = true;
};
// clang-format off
sreflect_struct(guid = "3bd3f46e-e8fd-41e8-9c94-c74e2d93141b")
CmdExec {
    // clang-format on
};
// clang-format off
sreflect_struct(guid = "098c17f7-d906-45ea-86d2-180d73a1cb40")
CmdSub {
    // clang-format on

    String    name       = {};
    skr_char8 short_name = {};
    String    help       = {};
    String    usage      = {};
};
} // namespace skr::attr

// cli output builder
namespace skr
{
namespace cli_style
{
static const char* clear = "\033[0m";

// style
static const char* bold         = "\033[1m";
static const char* no_bold      = "\033[22m";
static const char* underline    = "\033[4m";
static const char* no_underline = "\033[24m";
static const char* reverse      = "\033[7m";
static const char* no_reverse   = "\033[27m";

// front color
static const char* front_gray    = "\033[30m";
static const char* front_red     = "\033[31m";
static const char* front_green   = "\033[32m";
static const char* front_yellow  = "\033[33m";
static const char* front_blue    = "\033[34m";
static const char* front_magenta = "\033[35m";
static const char* front_cyan    = "\033[36m";
static const char* front_white   = "\033[37m";
} // namespace cli_style

struct CliOutputBuilder {
    // style
    inline CliOutputBuilder& style_clear()
    {
        content.append(cli_style::clear);
        return *this;
    }
    inline CliOutputBuilder& style_bold()
    {
        content.append(cli_style::bold);
        return *this;
    }
    inline CliOutputBuilder& style_no_bold()
    {
        content.append(cli_style::no_bold);
        return *this;
    }
    inline CliOutputBuilder& style_underline()
    {
        content.append(cli_style::underline);
        return *this;
    }
    inline CliOutputBuilder& style_no_underline()
    {
        content.append(cli_style::no_underline);
        return *this;
    }
    inline CliOutputBuilder& style_reverse()
    {
        content.append(cli_style::reverse);
        return *this;
    }
    inline CliOutputBuilder& style_no_reverse()
    {
        content.append(cli_style::no_reverse);
        return *this;
    }
    inline CliOutputBuilder& style_front_gray()
    {
        content.append(cli_style::front_gray);
        return *this;
    }
    inline CliOutputBuilder& style_front_red()
    {
        content.append(cli_style::front_red);
        return *this;
    }
    inline CliOutputBuilder& style_front_green()
    {
        content.append(cli_style::front_green);
        return *this;
    }
    inline CliOutputBuilder& style_front_yellow()
    {
        content.append(cli_style::front_yellow);
        return *this;
    }
    inline CliOutputBuilder& style_front_blue()
    {
        content.append(cli_style::front_blue);
        return *this;
    }
    inline CliOutputBuilder& style_front_magenta()
    {
        content.append(cli_style::front_magenta);
        return *this;
    }
    inline CliOutputBuilder& style_front_cyan()
    {
        content.append(cli_style::front_cyan);
        return *this;
    }
    inline CliOutputBuilder& style_front_white()
    {
        content.append(cli_style::front_white);
        return *this;
    }

    // content
    template <typename... Args>
    inline CliOutputBuilder& write(StringView fmt, Args... args)
    {
        format_to(content, fmt, args...);
        return *this;
    }
    template <typename... Args>
    inline CliOutputBuilder& line(StringView fmt, Args... args)
    {
        format_to(content, fmt, args...);
        next_line();
        return *this;
    }
    inline CliOutputBuilder& next_line()
    {
        content.append(u8"\n");
        return *this;
    }

    // dump
    inline void dump()
    {
        printf(content.c_str_raw());
    }

    String content;
};

} // namespace skr

namespace skr
{
struct CmdToken {
    enum class Type
    {
        Option,
        ShortOption,
        Name,
    };

    inline bool parse(StringView arg, CliOutputBuilder& out_err)
    {
        if (arg.starts_with(u8"--"))
        {
            type  = Type::Option;
            token = arg.subview(2);
        }
        else if (arg.starts_with(u8"-"))
        {
            type  = Type::ShortOption;
            token = arg.subview(1);
            if (token.size() > 1)
            {
                out_err
                    // error body
                    .style_bold()
                    .style_front_red()
                    .write(u8"option '")
                    .style_clear()
                    // option name
                    .style_front_green()
                    .write(arg)
                    .style_clear()
                    .style_bold()
                    // error body
                    .style_front_red()
                    .write(u8"' is not a short option! short option only support one character!")
                    .style_clear()
                    .next_line();
                return false;
            }
        }
        else
        {
            type  = Type::Name;
            token = arg;
        }
        return true;
    }

    inline bool is_any_option() const
    {
        return type == Type::Option || type == Type::ShortOption;
    }
    inline bool is_option() const
    {
        return type == Type::Option;
    }
    inline bool is_short_option() const
    {
        return type == Type::ShortOption;
    }
    inline bool is_name() const
    {
        return type == Type::Name;
    }

    inline String raw() const
    {
        String result;
        if (type == Type::Option)
        {
            result.append(u8"--");
        }
        else if (type == Type::ShortOption)
        {
            result.append(u8"-");
        }
        result.append(token);
        return result;
    }

    Type   type;
    String token;
};
struct SKR_CORE_API CmdOptionData {
    enum class EKind
    {
        None,
        Bool,
        Int8,
        Int16,
        Int32,
        Int64,
        UInt8,
        UInt16,
        UInt32,
        UInt64,
        Float,
        Double,
        String,
        StringVector,
    };

    bool init(attr::CmdOption config, TypeSignatureView type_sig, void* memory);

    // getter
    inline EKind                  kind() const { return _kind; }
    inline bool                   is_optional() const { return _is_optional; }
    inline const attr::CmdOption& config() const { return _config; }
    inline bool                   is_bool() const { return _kind == EKind::Bool; }
    inline bool                   is_uint() const { return _kind >= EKind::UInt8 && _kind <= EKind::UInt64; }
    inline bool                   is_int() const { return _kind >= EKind::Int8 && _kind <= EKind::Int64; }
    inline bool                   is_float() const { return _kind == EKind::Float || _kind == EKind::Double; }
    inline bool                   is_string() const { return _kind == EKind::String; }
    inline bool                   is_string_vector() const { return _kind == EKind::StringVector; }

    // getter
    inline bool is_required() const { return _config.is_required && !_is_optional; }

    // setter
    void set_value(bool v);
    void set_value(int64_t v);
    void set_value(uint64_t v);
    void set_value(double v);
    void set_value(String v);
    void add_value(String v);

private:
    // helper
    static EKind _solve_kind(const GUID& type_id);

    // as
    template <typename T>
    inline void _set(T&& v)
    {
        if (_is_optional)
        {
            _as<Optional<T>>() = std::forward<T>(v);
        }
        else
        {
            _as<T>() = std::forward<T>(v);
        }
    }
    template <typename T>
    inline void _set(const T& v)
    {
        if (_is_optional)
        {
            _as<Optional<T>>() = v;
        }
        else
        {
            _as<T>() = v;
        }
    }
    template <typename T>
    inline T& _as() const
    {
        return *reinterpret_cast<T*>(_memory);
    }

private:
    // attr
    attr::CmdOption _config = {};

    // native info
    EKind _kind        = EKind::None;
    bool  _is_optional = false;
    void* _memory      = nullptr;
};
struct SKR_CORE_API CmdNode {
    // basic info
    attr::CmdSub    config = {};
    const RTTRType* type   = nullptr;
    void*           object = nullptr;

    // options & invoke info
    Vector<CmdOptionData>       options     = {};
    ExportMethodInvoker<void()> exec_method = {};

    // sub command info
    Vector<CmdNode*> sub_cmds = {};

    // dtor
    ~CmdNode();

    // solve cmd
    bool solve();
    bool check_options();
    bool check_sub_commands();
    void print_help();
};
struct SKR_CORE_API CmdParser {
    // ctor & dtor
    CmdParser();
    ~CmdParser();

    // register command
    void main_cmd(const RTTRType* type, void* object, attr::CmdSub config = {});
    void sub_cmd(const RTTRType* type, void* object, attr::CmdSub config = {});

    // register command helper
    template <typename T>
    inline void main_cmd(T* object, attr::CmdSub config = {})
    {
        main_cmd(type_of<T>(), object, config);
    }
    template <typename T>
    inline void sub_cmd(T* object, attr::CmdSub config = {})
    {
        sub_cmd(type_of<T>(), object, config);
    }

    // parse
    void parse(int argc, char* argv[]);

private:
    // helper
    static span<const CmdToken> _find_option_param_pack(const Vector<CmdToken>& args, uint64_t option_idx);
    static span<const CmdToken> _find_rest_params_pack(const Vector<CmdToken>& args, uint64_t name_idx);
    static void                 _error_require_params(CliOutputBuilder& builder, const CmdToken& arg);
    static void                 _error_parse_params(CliOutputBuilder& builder, const CmdToken& arg, StringView param, StringView type);

private:
    CmdNode _root_cmd = {};
};
} // namespace skr