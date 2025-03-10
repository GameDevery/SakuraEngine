import * as gen from "@framework/generator"
import * as db from "@framework/database"
import * as ml from "@framework/meta_lang"
import { CodeBuilder } from "@framework/utils"

class RecordConfig extends ml.WithEnable {
}

class MethodConfig extends ml.WithEnable {
  @ml.value('string')
  getter: string = ""
  @ml.value('string')
  setter: string = ""
}

class _Gen {
  static body(record: db.Record) {
    record.generate_body_content.block(`/*content*/
void* self = nullptr;
const ${record.short_name}_VTable* vtable = nullptr;

/*ctor*/
template<typename T>
${record.short_name}(T& obj) noexcept
    : self(&obj)
    , vtable(${record.short_name}_VTableTraits<T>::get_vtable())
{
}
`)
  }
  static header(header: db.Header) {
    // gen concepts
    const concepts = new CodeBuilder();
    header.each_record((record) => {
      const record_config = record.gen_data.proxy as RecordConfig;
      if (!record_config.enable) return;

      concepts.line(`// concept of ${record.name}`)
      const concept_ns = record.namespace.length > 0 ? `proxy_concept::${record.namespace.join("::")}` : `proxy_concept`
      concepts.namespace_block(concept_ns, (b) => {
        record.methods.forEach((method) => {
          const method_config = method.gen_data.proxy as MethodConfig;
          if (!method_config.enable) return;

          concepts.block(`template <typename T>
  concept has_${method.short_name} = requires(${method.dump_const()} T t ${method.dump_params_with_comma()}) {
      { t.${method.short_name}( ${method.dump_params_name_only()} ) } -> std::convertible_to<${method.ret_type}>;
  };
  template <typename T>
  concept has_static_${method.short_name} = requires(${method.dump_const()} T t ${method.dump_params_with_comma()}) {
      { ${method.short_name}( &t ${method.dump_params_name_only_with_comma()} ) } -> std::convertible_to<${method.ret_type}>;
  };
  `)
          if (method_config.setter.length > 0) {
            concepts.block(`template <typename T>
  concept has_setter_${method.short_name} = requires(${method.dump_const()} T t ${method.dump_params_with_comma()}) {
      { t.${method_config.setter} = ${method.dump_params_name_only()} };
  };`)
          }
          if (method_config.getter.length > 0) {
            concepts.block(`template <typename T>
  concept has_getter_${method.short_name} = requires(${method.dump_const()} T t) {
      { t.${method_config.getter} } -> std::convertible_to<${method.ret_type}>;
  };`)
          }
        })
      })
    })

    // gen vtable
    let vtable = new CodeBuilder();
    header.each_record((record) => {
      const record_config = record.gen_data.proxy as RecordConfig;
      if (!record_config.enable) return;
      const concept_ns = record.namespace.length > 0 ? `proxy_concept::${record.namespace.join("::")}` : `proxy_concept`

      // gen vtable content
      vtable.namespace_block(record.namespace.join("::"), (b) => {
        // gen vtable content
        vtable.struct_block(`${record.short_name}_VTable`, (b) => {
          record.methods.forEach((method) => {
            const method_config = method.gen_data.proxy as MethodConfig;
            if (!method_config.enable) return;
            vtable.block(`    ${method.ret_type} (*${method.short_name})(${method.dump_const()} void* self ${method.dump_params_with_comma()}) = nullptr;`)
          })
        })

        // gen vtable traits
        vtable.line('template <typename T>')
        vtable.struct_block(`${record.short_name}_VTableTraits`, (b) => {
          // gen static getter helper
          record.methods.forEach((method) => {
            const method_config = method.gen_data.proxy as MethodConfig;
            if (!method_config.enable) return;
            const return_expr = method.has_return() ? 'return ' : ''
            vtable.block(`inline static ${method.ret_type} static_${method.short_name}(${method.dump_const()} void* self ${method.dump_params_with_comma()}) ${method.dump_noexcept()} {
  if constexpr (${concept_ns}::has_${method.short_name}<T>) {
      ${return_expr}static_cast<${method.dump_const()} T*>(self)->${method.short_name}(${method.dump_params_name_only()});
  } else if constexpr (${concept_ns}::has_static_${method.short_name}<T>) {
      ${return_expr}${method.short_name}(static_cast<${method.dump_const()} T*>(self) ${method.dump_params_name_only_with_comma()});
  %if method_proxy_data.setter:
  } else if constexpr (${concept_ns}::has_setter_${method.short_name}<T>) {
      static_assert(std::is_same_v<${method.ret_type}, void>, "Setter must return void");
      ${return_expr}static_cast<${method.dump_const()} T*>(self)->${method_config.setter} = ${method.dump_params_name_only()};
  %elif method_proxy_data.getter:
  } else if constexpr (${concept_ns}::has_getter_${method.short_name}<T>) {
      ${return_expr}static_cast<${method.dump_const()} T*>(self)->${method_config.getter};
  %endif
  }
}
`)
          })

          // gen vtable getter
          vtable.block(`static ${record.short_name}_VTable* get_vtable() {
    static ${record.short_name}_VTable _vtable = {`)
          record.methods.forEach((method) => {
            const method_config = method.gen_data.proxy as MethodConfig;
            if (!method_config.enable) return;
            vtable.line(`        &static_${method.short_name},`)
          })
          vtable.block(`};
    return &_vtable;
}`)

        })
      })
    })


    let traits = new CodeBuilder();
    header.each_record((record) => {
      const record_config = record.gen_data.proxy as RecordConfig;
      if (!record_config.enable) return;
      const concept_ns = record.namespace.length > 0 ? `proxy_concept::${record.namespace.join("::")}` : `proxy_concept`

      traits.block(`template <>
struct ProxyObjectTraits<${record.name}> {
    template <typename T>
    static constexpr bool validate()
    {
        return`)
      record.methods.forEach((method) => {
        const method_config = method.gen_data.proxy as MethodConfig;
        if (!method_config.enable) return;
        traits.line(`        ( // ${method.name}`)
        traits.line(`            ${concept_ns}::has_${method.short_name}<T>`)
        traits.line(`            || ${concept_ns}::has_static_${method.short_name}<T>`)
        if (method_config.setter.length > 0) {
          traits.line(`            || ${concept_ns}::has_setter_${method.short_name}<T>`)
        } else if (method_config.getter.length > 0) {
          traits.line(`            || ${concept_ns}::has_getter_${method.short_name}<T>`)
        }
        traits.line(`        )`)
        traits.line(`        &&`)
      })
      traits.block(`        true;
    }
};`)
    })

    header.gen_code.block(`// BEGIN PROXY GENERATED

// concepts
${concepts}

// vtable
${vtable}

// traits
namespace skr
{
${traits}
}
// END PROXY GENERATED
`)
  }
  static source(main_db: db.Module) {
    // gen member func impl
    let member_funcs = ""
    main_db.each_record((record) => {
      const record_config = record.gen_data.proxy as RecordConfig;
      if (!record_config.enable) return;
      member_funcs += `// ${record.name}\n`

      // gen func impl
      for (const method of record.methods) {
        const method_config = method.gen_data.proxy as MethodConfig;
        if (!record_config.enable) continue;
        if (method_config.enable) {
          member_funcs += `${method.ret_type} ${method.name}(${method.dump_params()}) ${method.dump_modifiers()}
{
    return vtable->${method.short_name}(self ${method.dump_params_name_only_with_comma()});
}`
        }
      }
    })

    main_db.gen_code.block(`// BEGIN PROXY GENERATED
${member_funcs}
// END PROXY GENERATED`)
  }
}

class ProxyGenerator extends gen.Generator {
  override gen_body(): void {
    this.main_module_db.each_record((record) => { _Gen.body(record); })
  }
  override gen(): void {
    // gen header
    this.main_module_db.headers.forEach((header) => { _Gen.header(header) })

    // gen source
    _Gen.source(this.main_module_db)
  }
}

export function load_generator(gm: gen.GenerateManager) {
  gm.add_generator("proxy", new ProxyGenerator())
}