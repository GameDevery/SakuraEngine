#include "SkrCore/cli.hpp"
#include "SkrCore/log.hpp"
#include <SkrContainers/optional.hpp>

// helper
namespace skr::cli_style
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

} // namespace skr::cli_style

// cmd option
namespace skr
{
bool CmdOptionData::init(attr::CmdOption config, TypeSignatureView type_sig, void* memory)
{
    if (type_sig.is_type())
    {
        // check decayed pointer
        if (type_sig.is_decayed_pointer())
        {
            SKR_LOG_FMT_ERROR(u8"option '{}' must be a value field!", config.name);
            return false;
        }

        // get type id
        GUID type_id;
        type_sig.jump_modifier();
        type_sig.read_type_id(type_id);

        // get type
        EKind kind = _solve_kind(type_id);
        if (kind == EKind::None)
        {
            SKR_LOG_FMT_ERROR(u8"unknown type for option '{}'!", config.name);
            return false;
        }

        _kind = kind;
    }
    else if (type_sig.is_generic_type())
    {
        // check decayed pointer
        if (type_sig.is_decayed_pointer())
        {
            SKR_LOG_FMT_ERROR(u8"option '{}' must be a value field!", config.name);
            return false;
        }

        // get generic id
        GUID     generic_id;
        uint32_t generic_param_count;
        type_sig.jump_modifier();
        type_sig.read_generic_type_id(generic_id, generic_param_count);

        if (generic_id == kOptionalGenericId)
        { // process optional
            _is_optional = true;

            // get type id
            if (type_sig.is_decayed_pointer())
            {
                SKR_LOG_FMT_ERROR(u8"option '{}' T of Optional<T> must be a value field!", config.name);
                return false;
            }

            if (type_sig.is_type())
            { // read inner type id
                GUID type_id;
                type_sig.jump_modifier();
                type_sig.read_type_id(type_id);
                EKind kind = _solve_kind(type_id);
                if (kind == EKind::None)
                {
                    SKR_LOG_FMT_ERROR(u8"unknown T of Optional<T> for option '{}'!", config.name);
                    return false;
                }
            }
            else
            {
                SKR_LOG_FMT_ERROR(u8"unknown T of Optional<T> for option '{}'!", config.name);
                return false;
            }
        }
        else if (generic_id == kVectorGenericId)
        {
            // get type id
            if (type_sig.is_type() && !type_sig.is_decayed_pointer())
            {
                // get typeid
                GUID type_id;
                type_sig.jump_modifier();
                type_sig.read_type_id(type_id);

                // check type
                if (type_id == type_id_of<String>())
                {
                    _kind = EKind::StringVector;
                }
            }
            else
            {
                SKR_LOG_FMT_ERROR(u8"unknown T of Vector<T> for option '{}'!", config.name);
                return false;
            }
        }
        else
        {
            SKR_LOG_FMT_ERROR(u8"unknown generic type for option '{}'!", config.name);
            return false;
        }
    }
    else
    {
        SKR_LOG_FMT_ERROR(u8"unknown type for option '{}'!", config.name);
        return false;
    }

    _memory = memory;
    _config = std::move(config);
    return true;
}

// setter
void CmdOptionData::set_value(bool v)
{
    SKR_ASSERT(_kind == EKind::Bool);
    _set<bool>(v);
}
void CmdOptionData::set_value(int64_t v)
{
    SKR_ASSERT(_kind == EKind::Int8 || _kind == EKind::Int16 || _kind == EKind::Int32 || _kind == EKind::Int64);
    switch (_kind)
    {
    case EKind::Int8:
        _set<int8_t>(static_cast<int8_t>(v));
        break;
    case EKind::Int16:
        _set<int16_t>(static_cast<int16_t>(v));
        break;
    case EKind::Int32:
        _set<int32_t>(static_cast<int32_t>(v));
        break;
    case EKind::Int64:
        _set<int64_t>(static_cast<int64_t>(v));
        break;
    default:
        SKR_UNREACHABLE_CODE()
        break;
    }
}
void CmdOptionData::set_value(uint64_t v)
{
    SKR_ASSERT(_kind == EKind::UInt8 || _kind == EKind::UInt16 || _kind == EKind::UInt32 || _kind == EKind::UInt64);
    switch (_kind)
    {
    case EKind::UInt8:
        _set<uint8_t>(static_cast<uint8_t>(v));
        break;
    case EKind::UInt16:
        _set<uint16_t>(static_cast<uint16_t>(v));
        break;
    case EKind::UInt32:
        _set<uint32_t>(static_cast<uint32_t>(v));
        break;
    case EKind::UInt64:
        _set<uint64_t>(static_cast<uint64_t>(v));
        break;
    default:
        SKR_UNREACHABLE_CODE()
        break;
    }
}
void CmdOptionData::set_value(double v)
{
    SKR_ASSERT(_kind == EKind::Float || _kind == EKind::Double);
    switch (_kind)
    {
    case EKind::Float:
        _set<float>(static_cast<float>(v));
        break;
    case EKind::Double:
        _set<double>(static_cast<double>(v));
        break;
    default:
        SKR_UNREACHABLE_CODE()
        break;
    }
}
void CmdOptionData::set_value(String v)
{
    SKR_ASSERT(_kind == EKind::String);
    if (_kind == EKind::String)
    {
        _set<String>(std::move(v));
    }
    else
    {
        SKR_UNREACHABLE_CODE()
    }
}
void CmdOptionData::add_value(String v)
{
    SKR_ASSERT(_kind == EKind::StringVector);
    if (_kind == EKind::StringVector)
    {
        _as<Vector<String>>().add(std::move(v));
    }
    else
    {
        SKR_UNREACHABLE_CODE()
    }
}

// helper
CmdOptionData::EKind CmdOptionData::_solve_kind(const GUID& type_id)
{
    switch (type_id.get_hash())
    {
    case type_id_of<bool>().get_hash():
        return EKind::Bool;
    case type_id_of<int8_t>().get_hash():
        return EKind::Int8;
    case type_id_of<int16_t>().get_hash():
        return EKind::Int16;
    case type_id_of<int32_t>().get_hash():
        return EKind::Int32;
    case type_id_of<int64_t>().get_hash():
        return EKind::Int64;
    case type_id_of<uint8_t>().get_hash():
        return EKind::UInt8;
    case type_id_of<uint16_t>().get_hash():
        return EKind::UInt16;
    case type_id_of<uint32_t>().get_hash():
        return EKind::UInt32;
    case type_id_of<uint64_t>().get_hash():
        return EKind::UInt64;
    case type_id_of<float>().get_hash():
        return EKind::Float;
    case type_id_of<double>().get_hash():
        return EKind::Double;
    case type_id_of<String>().get_hash():
        return EKind::String;
    };
    return EKind::None;
}
} // namespace skr

// cmd node
namespace skr
{
// dtor
CmdNode::~CmdNode()
{
    for (auto& sub_cmd : sub_cmds)
    {
        SkrDelete(sub_cmd);
    }
}

// solve cmd
bool CmdNode::solve()
{
    bool failed = false;
    type->each_field([&](const RTTRFieldData* field, const RTTRType* owner_type) {
        // find option
        auto found_option_attr_ref = field->attrs.find_if([&](const Any& v) { return v.type_is<attr::CmdOption>(); });
        if (found_option_attr_ref)
        {
            // solve config
            attr::CmdOption option_attr = found_option_attr_ref.ref().as<attr::CmdOption>();
            option_attr.name            = (!option_attr.name.is_empty()) ? option_attr.name : field->name;

            // get field address
            void* owner_obj     = type->cast_to_base(owner_type->type_id(), object);
            void* field_address = field->get_address(owner_obj);

            // add option
            auto& new_option = options.add_default().ref();
            failed |= !new_option.init(std::move(option_attr), field->type, field_address);
            return;
        }

        // find sub command
        auto found_sub_cmd_attr_ref = field->attrs.find_if([&](const Any& v) { return v.type_is<attr::CmdSub>(); });
        if (found_sub_cmd_attr_ref)
        {
            // solve config
            attr::CmdSub sub_cmd_attr = found_sub_cmd_attr_ref.ref().as<attr::CmdSub>();
            sub_cmd_attr.name         = (!sub_cmd_attr.name.is_empty()) ? sub_cmd_attr.name : field->name;
            auto field_type_sig       = field->type.view();

            // check signature
            if (!field_type_sig.is_type())
            {
                SKR_LOG_FMT_ERROR(u8"sub command must be a type!");
                return;
            }
            if (field_type_sig.is_decayed_pointer())
            {
                SKR_LOG_FMT_ERROR(u8"sub command must be a value field!");
                return;
            }

            // get type
            GUID type_id;
            field_type_sig.jump_modifier();
            field_type_sig.read_type_id(type_id);
            const RTTRType* sub_cmd_type = get_type_from_guid(type_id);
            if (!sub_cmd_type)
            {
                SKR_LOG_FMT_ERROR(u8"sub command type not found!");
                return;
            }

            // get field address
            void* owner_obj     = sub_cmd_type->cast_to_base(owner_type->type_id(), object);
            void* field_address = field->get_address(owner_obj);

            // create sub command
            auto sub_cmd = SkrNew<CmdNode>();

            // setup basic info
            sub_cmd->config = std::move(sub_cmd_attr);
            sub_cmd->type   = sub_cmd_type;
            sub_cmd->object = field_address;

            // solve sub cmd
            failed |= !sub_cmd->solve();

            // register sub command
            sub_cmds.push_back(sub_cmd);
            return;
        }

        // clang-format off
    }, { .include_bases = true });
    // clang-format on

    failed |= check_sub_commands();
    return !failed;
}
bool CmdNode::check_sub_commands()
{
    Map<String, CmdNode*>    sub_cmd_map;
    Map<skr_char8, CmdNode*> sub_cmd_short_map;

    bool no_error = true;

    for (auto& sub_cmd : sub_cmds)
    {
        // check name
        if (auto found = sub_cmd_map.find(sub_cmd->config.name))
        {
            SKR_LOG_FMT_ERROR(u8"duplicate sub command name: {}", sub_cmd->config.name);
            no_error = false;
        }
        sub_cmd_map.add(sub_cmd->config.name, sub_cmd);

        // check short name
        if (auto found = sub_cmd_short_map.find(sub_cmd->config.short_name))
        {
            SKR_LOG_FMT_ERROR(u8"duplicate sub command short name: {}", sub_cmd->config.short_name);
            no_error = false;
        }
        sub_cmd_short_map.add(sub_cmd->config.short_name, sub_cmd);
    }

    return no_error;
}
void CmdNode::print_help()
{
    // print self usage
    printf(
        "%susage:%s %s%s%s\n",
        cli_style::bold,
        cli_style::clear,
        cli_style::front_blue,
        config.help.c_str_raw(),
        cli_style::clear
    );
    if (!options.is_empty())
    {
        printf(
            "%soptions:%s\n",
            cli_style::bold,
            cli_style::clear
        );
        for (auto& option : options)
        {
            if (option.config().short_name != 0)
            {
                printf(
                    "%s  -%c, --%s:%s %s\n",
                    cli_style::front_green,
                    option.config().short_name,
                    option.config().name.c_str_raw(),
                    cli_style::clear,
                    option.config().help.c_str_raw()
                );
            }
            else
            {
                printf(
                    "%s  --%s:%s %s\n",
                    cli_style::front_green,
                    option.config().name.c_str_raw(),
                    cli_style::clear,
                    option.config().help.c_str_raw()
                );
            }
        }
    }

    // print subcommands
    if (!sub_cmds.is_empty())
    {
        printf(
            "%ssub commands:%s\n",
            cli_style::bold,
            cli_style::clear
        );
        for (auto& sub_cmd : sub_cmds)
        {
            printf(
                "%s  %s:%s %s\n",
                cli_style::front_cyan,
                sub_cmd->config.name.c_str_raw(),
                cli_style::clear,
                sub_cmd->config.help.c_str_raw()
            );
        }
    }
}
} // namespace skr

// cmd parser
namespace skr
{
// ctor & dtor
CmdParser::CmdParser()
{
}
CmdParser::~CmdParser()
{
}

// register command
void CmdParser::main_cmd(const RTTRType* type, void* object, attr::CmdSub config)
{
    if (_root_cmd.object != nullptr)
    {
        SKR_LOG_FMT_ERROR(u8"main command has registered!");
        return;
    }

    // fill main command info
    _root_cmd.config = config;
    _root_cmd.type   = type;
    _root_cmd.object = object;
    _root_cmd.solve();
}
void CmdParser::sub_cmd(const RTTRType* type, void* object, attr::CmdSub config)
{
    // fill sub command info
    auto* sub_cmd   = SkrNew<CmdNode>();
    sub_cmd->config = config;
    sub_cmd->type   = type;
    sub_cmd->object = object;
    sub_cmd->solve();
    _root_cmd.sub_cmds.push_back(sub_cmd);
    _root_cmd.check_sub_commands();
}

// parse
void CmdParser::parse(int argc, char* argv[])
{
    // parse args
    String           program_name = String::From(argv[0]);
    Vector<CmdToken> args         = {};
    args.reserve(argc);
    for (int i = 1; i < argc; ++i)
    {
        args.add(StringView{ reinterpret_cast<const skr_char8*>(argv[i]) });
    }

    uint32_t current_idx = 0;

    // solve sub command
    CmdNode* current_cmd = &_root_cmd;
    while (current_idx < args.size() && args[current_idx].type == CmdToken::Type::Name)
    {
        const auto& token = args[current_idx].token;
        auto        found = current_cmd->sub_cmds.find_if([&](const CmdNode* cmd) {
            String short_name;
            short_name.add(cmd->config.short_name);
            return short_name == token || cmd->config.name == token;
        });
        if (found)
        {
            current_cmd = found.ref();
            ++current_idx;
        }
        else
        {
            SKR_LOG_FMT_ERROR(u8"unknown sub command: {}", token.c_str());
            return;
        }
    }

    // help command
    {
        auto found = args.find_if([&](const CmdToken& token) {
            return (token.type == CmdToken::Type::ShortOption && token.token == u8"h") ||
                   (token.type == CmdToken::Type::Option && token.token == u8"help");
        });
        if (found)
        {
            current_cmd->print_help();
            return;
        }
    }

    // parse options
    while (current_idx < args.size())
    {
        ++current_idx;
    }
}
} // namespace skr