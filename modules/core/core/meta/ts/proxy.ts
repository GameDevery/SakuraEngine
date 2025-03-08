import * as gen from "@framework/generator"
import * as db from "@framework/database"
import * as cpp from "@framework/cpp_types"

class RecordData {
  enable: boolean = false
}

class MethodData {
  enable: boolean = false
  getter: string = ""
  setter: string = ""
} 

class ProxyGenerator extends gen.Generator {

  override gen_body(): void {
    this.owner.project_db.main_module.each_record((record, _header) => {
      const record_data = record.gen_data.proxy as RecordData
      if (record_data.enable) {
        record.generate_body_content += 
`/*content*/
void* self = nullptr;
const ${record.short_name}_VTable* vtable = nullptr;

/*ctor*/
template<typename T>
${record.short_name}(T& obj) noexcept
    : self(&obj)
    , vtable(${record.short_name}_VTableTraits<T>::get_vtable())
{
}
`
      }
    })
  }
  override gen(): void {
    // gen header
    for (const header_db of this.owner.project_db.main_module.headers) {
      this.owner.append_content(
        header_db.output_header_path,
`// BEGIN PROXY GENERATED
${this.#gen_vtable(header_db)}
namespace skr
{
${this.#gen_traits(header_db)}
}
// END PROXY GENERATED
`
      )
    }
  }
  #gen_vtable(header: db.Header) : string {
    let result = ""
    for (const record of header.records) {
      const record_data = record.gen_data.proxy as RecordData
      if (!record_data.enable) continue;
      if (record.namespace.length > 0) {
        result +=
`namespace ${record.namespace.join("::")} 
{`
      }

      result +=
`struct ${record.short_name}_VTable {
${this.#gen_vtable_methods(record)}
}
static ${record.short_name}_VTable* get_vtable() {
    static ${record.short_name}_VTable _vtable = {
${this.#gen_get_vtable_content(record)}
    };
    return &_vtable;
}
`
      if (record.namespace.length > 0) {
        result += `}`
      }
    }
    return result
  }
  #gen_vtable_methods(record: cpp.Record) : string{
    let result = ""
    for (const method of record.methods) {
      const method_data = method.gen_data.proxy as MethodData;
      if (method_data.enable) {
        const _dump_content = () => {
          let result = ""
          if (method_data.getter.length > 0) {
            result =
`    if constexpr(validate_method(static_cast<${method.dump_const()} T*>(0) ${method.dump_params_name_only_with_comma()}))
    {
        return static_cast<${method.dump_const()} T*>(self)->${method.short_name}(${method.dump_params_name_only()});
    }
    else if constexpr(validate_static_method(${method.dump_params_name_only()}))
    {
        return ${method.short_name}(static_cast<${method.dump_const()} T*>(self) ${method.dump_params_name_only_with_comma()});
    }
    else
    {
        return static_cast<${method.dump_const()} T*>(self)->${method_data.getter};
    }`
          }
          else if (method_data.setter.length > 0) {
            result =
`    static_assert(std::is_same_v<${method.ret_type}, void>, "Setter must return void");
    if constexpr(validate_method(static_cast<${method.dump_const()} T*>(0) ${method.dump_params_name_only_with_comma()}))
    {
        static_cast<${method.dump_const()} T*>(self)->${method.short_name}(${method.dump_params_name_only()});
    }
    else if constexpr(validate_static_method(${method.dump_params_name_only()}))
    {
        ${method.short_name}(static_cast<${method.dump_const()} T*>(self) ${method.dump_params_name_only_with_comma()});
    }
    else
    {
        static_cast<${method.dump_const()} T*>(self)->${method_data.setter} = ${method.dump_params_name_only()};
    }
`
          }
          else {
            result =
`    if constexpr(validate_method(static_cast<${method.dump_const()} T*>(0) ${method.dump_params_name_only_with_comma()}))
    {
        return static_cast<${method.dump_const()} T*>(self)->${method.short_name}(${method.dump_params_name_only()});
    }
    else
    {
        return ${method.short_name}(static_cast<${method.dump_const()} T*>(self) ${method.dump_params_name_only_with_comma()});
    }
`
          }
          return result
        }

        result += 
`inline static ${method.ret_type} static_${method.short_name}(${method.dump_const()} void* self ${method.dump_params_with_comma()}) ${method.dump_noexcept()}
{
    [[maybe_unused]] auto validate_method = SKR_VALIDATOR((auto obj, auto... args), obj->${method.short_name}(args...));
    [[maybe_unused]] auto validate_static_method = SKR_VALIDATOR((auto... args), ${method.short_name}(static_cast<${method.dump_const()} T*>(0), args...));
${_dump_content()}
}
`
      }
    }
    return result
  }
  #gen_get_vtable_content(record: cpp.Record): string {
    let result = ""
    for (const method of record.methods) {
      const method_data = method.gen_data.proxy as MethodData;
      if (method_data.enable) {
        result += `&static_${method.short_name},`
      }
    }
    return result;
  }
  #gen_traits(header: db.Header): string {
    let result = ""
    for (const record of header.records) {
      result +=
`template <>
struct ProxyObjectTraits<${record.name}> {
    template <typename T>
    static constexpr bool validate()
    {
        return
${this.#gen_traits_methods(record)}
        true;
    }
};
`
    }
    return result
  }
  #gen_traits_methods(record: cpp.Record): string {
    let result = ""
    for (const method of record.methods) {
      const method_data = method.gen_data.proxy as MethodData;
      if (method_data.enable) {
        const getter_setter_require = () => {
          let result = ""
          if (method_data.getter.length > 0) {
            result +=
`            || requires(${method.dump_const()} T t ${method.dump_params_with_comma()})
            {
                { t.${method_data.setter} = ${method.dump_params_name_only()} };
            }
`
          } else if (method_data.setter.length > 0) {
            result += 
`            || requires(${method.dump_const()} T t)
            {
                { t.${method_data.getter} } -> std::convertible_to<${method.ret_type}>;
            }
`
          }
          return result
        }

        result +=
`(   // ${method.ret_type} ${record.name}::${method.short_name}(${method.dump_params()}) ${method.dump_const()} ${method.dump_noexcept()}
            requires(${method.dump_const()} T t ${method.dump_params_with_comma()})
            {
                { t.${method.short_name}( ${method.dump_params_name_only()} ) } -> std::convertible_to<${method.ret_type}>;
            }
            || requires(${method.dump_const()} T t ${method.dump_params_with_comma()})
            {
                { ${method.short_name}( &t ${method.dump_params_name_only_with_comma()} ) } -> std::convertible_to<${method.ret_type}>;
            }
            ${getter_setter_require()}
        )
`
      }
    }
    return result
  }
}

export function load_generator(gm: gen.GenerateManager) {
  gm.add_generator("proxy", new ProxyGenerator())
}