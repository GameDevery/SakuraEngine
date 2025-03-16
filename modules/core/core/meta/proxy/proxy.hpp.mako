// BEGIN PROXY GENERATED

// concepts
%for record in records:
<%
record_proxy_data=record.generator_data["proxy"]
namespace = f"proxy_concept::{record.namespace}" if record.namespace else "proxy_concept"
%>\
// concept of ${record.name}
namespace ${namespace}
{
%for method in record_proxy_data.methods:
<%method_proxy_data=method.generator_data["proxy"]%>\
template <typename T>
concept has_${method.short_name} = requires(${method.dump_const()} T t ${method.dump_params_with_comma()}) {
    { t.${method.short_name}( ${method.dump_params_name_only()} ) } -> std::convertible_to<${method.ret_type}>;
};
template <typename T>
concept has_static_${method.short_name} = requires(${method.dump_const()} T t ${method.dump_params_with_comma()}) {
    { ${method.short_name}( &t ${method.dump_params_name_only_with_comma()} ) } -> std::convertible_to<${method.ret_type}>;
};
%if method_proxy_data.setter:
template <typename T>
concept has_setter_${method.short_name} = requires(${method.dump_const()} T t ${method.dump_params_with_comma()}) {
    { t.${method_proxy_data.setter} = ${method.dump_params_name_only()} };
};
%elif method_proxy_data.getter:
template <typename T>
concept has_getter_${method.short_name} = requires(${method.dump_const()} T t) {
    { t.${method_proxy_data.getter} } -> std::convertible_to<${method.ret_type}>;
};
%endif
%endfor
}
%endfor

// vtable
%for record in records:
<%record_proxy_data=record.generator_data["proxy"]%>\
%if record.namespace:
namespace ${record.namespace}
{
%endif
struct ${record.short_name}_VTable {
%for method in record_proxy_data.methods:
<%method_proxy_data=method.generator_data["proxy"]%>\
    ${method.ret_type} (*${method.short_name})(${method.dump_const()} void* self ${method.dump_params_with_comma()}) = nullptr;
%endfor
};
template <typename T>
struct ${record.short_name}_VTableTraits {
%for method in record_proxy_data.methods:
<%
method_proxy_data=method.generator_data["proxy"]
proxy_ns = f"proxy_concept::{record.namespace}" if record.namespace else "proxy_concept"
return_expr = "return " if method.has_return() else ""
%>\
    inline static ${method.ret_type} static_${method.short_name}(${method.dump_const()} void* self ${method.dump_params_with_comma()}) ${method.dump_noexcept()} {
        if constexpr (${proxy_ns}::has_${method.short_name}<T>) {
            ${return_expr}static_cast<${method.dump_const()} T*>(self)->${method.short_name}(${method.dump_params_name_only()});
        } else if constexpr (${proxy_ns}::has_static_${method.short_name}<T>) {
            ${return_expr}${method.short_name}(static_cast<${method.dump_const()} T*>(self) ${method.dump_params_name_only_with_comma()});
    %if method_proxy_data.setter:
        } else if constexpr (${proxy_ns}::has_setter_${method.short_name}<T>) {
            static_assert(std::is_same_v<${method.ret_type}, void>, "Setter must return void");
            ${return_expr}static_cast<${method.dump_const()} T*>(self)->${method_proxy_data.setter} = ${method.dump_params_name_only()};
    %elif method_proxy_data.getter:
        } else if constexpr (${proxy_ns}::has_getter_${method.short_name}<T>) {
            ${return_expr}static_cast<${method.dump_const()} T*>(self)->${method_proxy_data.getter};
    %endif
        }
    }
%endfor
static ${record.short_name}_VTable* get_vtable() {
    static ${record.short_name}_VTable _vtable = {
%for method in record_proxy_data.methods:
        &static_${method.short_name},
%endfor
    };
    return &_vtable;
}
};
%if record.namespace:
}
%endif
%endfor

// traits
namespace skr
{
%for record in records:
<%record_proxy_data=record.generator_data["proxy"]%>\
template <>
struct ProxyObjectTraits<${record.name}> {
    template <typename T>
    static constexpr bool validate()
    {
        return
        %for method in record_proxy_data.methods:
<%
method_proxy_data=method.generator_data["proxy"]
proxy_ns = f"proxy_concept::{record.namespace}" if record.namespace else "proxy_concept"
%>\
        ( // ${method.name}
            ${proxy_ns}::has_${method.short_name}<T>
            || ${proxy_ns}::has_static_${method.short_name}<T>
        %if method_proxy_data.setter:
            || ${proxy_ns}::has_setter_${method.short_name}<T>
        %elif method_proxy_data.getter:
            || ${proxy_ns}::has_getter_${method.short_name}<T>
        %endif
        )
        &&
        %endfor
        true;
    }
};
%endfor
}
// END PROXY GENERATED
