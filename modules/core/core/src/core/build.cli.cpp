#include "SkrCore/cli.hpp"
#include "SkrCore/log.hpp"
#include <SkrContainers/optional.hpp>

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
void CmdNode::solve()
{
    type->each_field([&](const RTTRFieldData* field, const RTTRType* owner_type) {
        // find option
        auto found_option = field->attrs.find_if([&](const Any& v) { return v.type_is<attr::CmdOption>(); });
        if (found_option)
        {
            const auto& option_attr = found_option.ref().cast<attr::CmdOption>();
            auto        signature   = field->type.view();

            // check option type
            if (signature.is_decayed_pointer())
            {
                SKR_LOG_FMT_ERROR(u8"option must be a value field!");
                return;
            }
            const RTTRType* option_type          = nullptr;
            void*           option_field_address = nullptr;
            OptionalBase*   optional_base        = nullptr;
            if (signature.is_generic_type())
            {
                // check generic
                GUID     generic_id;
                uint32_t data_count;
                signature.jump_modifier();
                signature.read_generic_type_id(generic_id, data_count);
                if (generic_id != kOptionalGenericId)
                {
                    SKR_LOG_FMT_ERROR(u8"unknown generic type for option!");
                    return;
                }

                // get type
                GUID type_id;
                if (signature.is_decayed_pointer())
                {
                    SKR_LOG_FMT_ERROR(u8"option must be a value field!");
                    return;
                }
                signature.jump_modifier();
                signature.read_type_id(type_id);
                option_type = get_type_from_guid(type_id);
                if (!option_type)
                {
                    SKR_LOG_FMT_ERROR(u8"type not found!");
                    return;
                }

                // get field address
                void* owner_obj      = option_type->cast_to_base(owner_type->type_id(), object);
                void* field_address  = field->get_address(owner_obj);
                optional_base        = reinterpret_cast<OptionalBase*>(field_address);
                option_field_address = reinterpret_cast<uint8_t*>(field_address) + optional_base->_padding;
            }
            else if (signature.is_type())
            {
                // get type
                GUID type_id;
                signature.jump_modifier();
                signature.read_type_id(type_id);
                option_type = get_type_from_guid(type_id);
                if (!option_type)
                {
                    SKR_LOG_FMT_ERROR(u8"type not found!");
                    return;
                }
            }
            else
            {
                SKR_LOG_FMT_ERROR(u8"unknown type for option!");
                return;
            }

            // add option
            auto& option_info         = options.add_default().ref();
            option_info.name          = (!option_attr->name.is_empty()) ? option_attr->name : field->name;
            option_info.short_name    = option_attr->short_name;
            option_info.help          = option_attr->help;
            option_info.type          = option_type;
            option_info.object        = option_field_address;
            option_info.optional_base = optional_base;
        }

        // find sub command
        auto found_sub_cmd = field->attrs.find_if([&](const Any& v) { return v.type_is<attr::CmdSub>(); });
        if (found_sub_cmd)
        {
            const auto& sub_cmd_attr = found_sub_cmd.ref().cast<attr::CmdSub>();
            auto        signature    = field->type.view();

            // check signature
            if (!signature.is_type())
            {
                SKR_LOG_FMT_ERROR(u8"sub command must be a type!");
                return;
            }
            if (signature.is_decayed_pointer())
            {
                SKR_LOG_FMT_ERROR(u8"sub command must be a value field!");
                return;
            }

            // get type
            GUID type_id;
            signature.jump_modifier();
            signature.read_type_id(type_id);
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
            sub_cmd->name       = (!sub_cmd_attr->name.is_empty()) ? sub_cmd_attr->name : field->name;
            sub_cmd->short_name = sub_cmd_attr->short_name;
            sub_cmd->help       = sub_cmd_attr->help;
            sub_cmd->type       = sub_cmd_type;
            sub_cmd->object     = field_address;

            // solve sub cmd
            sub_cmd->solve();

            // register sub command
            sub_cmds.push_back(sub_cmd);
        }

        // clang-format off
    }, { .include_bases = true });
    // clang-format on

    check_sub_commands();
}
bool CmdNode::check_sub_commands()
{
    Map<String, CmdNode*>    sub_cmd_map;
    Map<skr_char8, CmdNode*> sub_cmd_short_map;

    bool no_error = true;

    for (auto& sub_cmd : sub_cmds)
    {
        // check name
        if (auto found = sub_cmd_map.find(sub_cmd->name))
        {
            SKR_LOG_FMT_ERROR(u8"duplicate sub command name: {}", sub_cmd->name.c_str());
        }
        sub_cmd_map.add(sub_cmd->name, sub_cmd);

        // check short name
        if (auto found = sub_cmd_short_map.find(sub_cmd->short_name))
        {
            SKR_LOG_FMT_ERROR(u8"duplicate sub command short name: {}", sub_cmd->short_name);
        }
        sub_cmd_short_map.add(sub_cmd->short_name, sub_cmd);
    }

    return no_error;
}
void CmdNode::print_help()
{
    // print self usage
    printf("usage: %s\n", help.c_str_raw());
    if (!options.is_empty())
    {
        printf("options:\n");
        for (auto& option : options)
        {
            if (option.short_name != 0)
            {
                printf("  -%c, --%s: %s\n", option.short_name, option.name.c_str_raw(), option.help.c_str_raw());
            }
            else
            {
                printf("  --%s: %s\n", option.name.c_str_raw(), option.help.c_str_raw());
            }
        }
    }

    // print subcommands
    if (!sub_cmds.is_empty())
    {
        printf("sub commands:\n");
        for (auto& sub_cmd : sub_cmds)
        {
            printf("  %s: %s\n", sub_cmd->name.c_str_raw(), sub_cmd->help.c_str_raw());
        }
    }
}

// ctor & dtor
CmdParser::CmdParser()
{
}
CmdParser::~CmdParser()
{
}

// register command
void CmdParser::main_cmd(const RTTRType* type, void* object, CmdConfig config)
{
    if (_root_cmd.object != nullptr)
    {
        SKR_LOG_FMT_ERROR(u8"main command has registered!");
        return;
    }

    // fill main command info
    _root_cmd.type   = type;
    _root_cmd.object = object;
    _root_cmd.help   = std::move(config.help);
    _root_cmd.solve();
}
void CmdParser::sub_cmd(const RTTRType* type, void* object, CmdConfig config)
{
    // fill sub command info
    auto* sub_cmd       = SkrNew<CmdNode>();
    sub_cmd->name       = std::move(config.name);
    sub_cmd->short_name = config.short_name;
    sub_cmd->help       = std::move(config.help);
    sub_cmd->type       = type;
    sub_cmd->object     = object;
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
            short_name.add(cmd->short_name);
            return short_name == token || cmd->name == token;
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