#pragma once
#include "SkrBase/config.h"
#include "SkrBase/meta.h"
#include "SkrContainersDef/string.hpp"
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

    String    name       = {};
    skr_char8 short_name = {};
    String    help       = {};
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
struct CmdConfig {
    String    name       = {};
    skr_char8 short_name = {};
    String    help       = {};
};
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
struct SKR_CORE_API CmdNode {
    struct OptionInfo {
        String    name       = {};
        skr_char8 short_name = {};
        String    help       = {};

        const RTTRType* type          = nullptr;
        void*           object        = nullptr;
        OptionalBase*   optional_base = nullptr;
    };

    // basic info
    String          name       = {};
    skr_char8       short_name = {};
    String          help       = {};
    const RTTRType* type       = nullptr;
    void*           object     = nullptr;

    // options & invoke info
    Vector<OptionInfo>    options     = {};
    const RTTRMethodData* exec_method = {};

    // sub command info
    Vector<CmdNode*> sub_cmds = {};

    // dtor
    ~CmdNode();

    // solve cmd
    void solve();
    bool check_sub_commands();
    void print_help();
};
struct SKR_CORE_API CmdParser {
    // ctor & dtor
    CmdParser();
    ~CmdParser();

    // register command
    void main_cmd(const RTTRType* type, void* object, CmdConfig config = {});
    void sub_cmd(const RTTRType* type, void* object, CmdConfig config = {});

    // register command helper
    template<typename T>
    inline void main_cmd(T* object, CmdConfig config = {})
    {
        main_cmd(type_of<T>(), object, config);
    }
    template<typename T>
    inline void sub_cmd(T* object, CmdConfig config = {})
    {
        sub_cmd(type_of<T>(), object, config);
    }

    // parse
    void parse(int argc, char* argv[]);

private:
    CmdNode _root_cmd = {};
};
} // namespace skr