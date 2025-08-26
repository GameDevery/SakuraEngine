#include <SkrV8/ts_def_builder.hpp>
#include <SkrV8/v8_virtual_module.hpp>

namespace skr
{
// generate
String TSDefBuilder::generate_global()
{
    SKR_ASSERT(virtual_module && "please specify module first");

    _result.clear();
    $line(u8"export {{}};");
    $line(u8"declare global {{");
    $indent([&] {
        _gen();
    });
    $line(u8"}}");
    return _result;
}
String TSDefBuilder::generate_module(StringView module_name)
{
    SKR_ASSERT(virtual_module && "please specify module first");

    _result.clear();
    $line(u8"declare module \"{}\" {{", module_name);
    $indent([&] {
        _gen();
    });
    $line(u8"}}");
    return _result;
}

// helper
String TSDefBuilder::type_name_with_ns(const V8BindTemplate* bind_tp) const
{
    auto ns = virtual_module->bind_tp_to_ns(bind_tp);
    if (ns.is_empty())
    {
        return bind_tp->get_ts_type_name();
    }
    else
    {
        ns.replace(u8"::", u8".");
        return ns;
    }
}
String TSDefBuilder::params_signature(const Vector<V8BTDataParam>& params) const
{
    String result;
    for (size_t i = 0; i < params.size(); ++i)
    {
        auto& param = params[i];

        // filter pure out
        if (param.inout_flag == ERTTRParamFlag::Out)
        {
            continue;
        }

        // push comma
        if (i > 0)
        {
            result.append(u8", ");
        }

        // push inout flags
        if (param.inout_flag == ERTTRParamFlag::InOut)
        {
            result.append(u8"/*inout*/");
        }

        // push param name
        result.append(param.rttr_data->name);
        if (param.bind_tp->ts_is_nullable())
        {
            result.append(u8"?");
        }

        // push type specifier
        result.append(u8": ");

        // push type
        result.append(type_name_with_ns(param.bind_tp));
    }
    return result;
}
String TSDefBuilder::return_signature(const V8BTDataFunctionBase& func_data) const
{
    if (func_data.return_count == 0)
    {
        return u8"void";
    }
    else if (func_data.return_count == 1)
    {
        if (func_data.return_data.is_void)
        {
            for (auto& param : func_data.params_data)
            {
                if (param.appare_in_return)
                {
                    return format(
                        u8"{} /*{}*/",
                        type_name_with_ns(param.bind_tp),
                        param.rttr_data->name
                    );
                }
            }
            SKR_UNREACHABLE_CODE();
            return {};
        }
        else
        {
            return type_name_with_ns(func_data.return_data.bind_tp);
        }
    }
    else
    {
        String result;
        result.append(u8"[");
        if (!func_data.return_data.is_void)
        {
            result.append(
                format(
                    u8"{} /*[return]*/",
                    type_name_with_ns(func_data.return_data.bind_tp)
                )
            );
            result.append(u8", ");
        }
        for (auto& param : func_data.params_data)
        {
            if (param.appare_in_return)
            {
                result.append(
                    format(
                        u8"{} /*{}*/",
                        type_name_with_ns(param.bind_tp),
                        param.rttr_data->name
                    )
                );
                result.append(u8", ");
            }
        }
        // remove last comma
        result.first(result.length_buffer() - 2);
        result.append(u8"]");
        return result;
    }
}

// helpers
void TSDefBuilder::_gen()
{
    auto& root_node = virtual_module->root_node();
    for (const auto& [node_name, child] : root_node.children())
    {
        _gen_ns_node(*child);
    }
}
void TSDefBuilder::_gen_ns_node(const V8VirtualModuleNode& node)
{
    if (node.is_namespace())
    {
        $line(u8"export namespace {} {{", node.name());
        $indent([&]() {
            for (const auto& [node_name, child] : node.children())
            {
                _gen_ns_node(*child);
            }
        });
        $line(u8"}}");
    }
    else
    {
        auto bind_tp = node.bind_tp();
        bind_tp->dump_ts_def(*this);
    }
}
} // namespace skr