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
};
} // namespace skr::attr

namespace skr
{
struct CmdToken {
    enum class Type
    {
        Option,
        ShortOption,
        Name,
    };

    inline CmdToken(StringView arg)
    {
        if (arg.starts_with(u8"-"))
        {
            type  = Type::ShortOption;
            token = arg.subview(1);
        }
        else if (arg.starts_with(u8"--"))
        {
            type  = Type::Option;
            token = arg.subview(2);
        }
        else
        {
            type  = Type::Name;
            token = arg;
        }
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
    CmdNode _root_cmd = {};
};
} // namespace skr