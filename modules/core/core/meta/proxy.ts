import * as gen from "@framework/generator";
import * as db from "@framework/database";
import * as ml from "@framework/meta_lang";
import { CodeBuilder } from "@framework/utils";

class RecordConfig extends ml.WithEnable {
}

class MethodConfig extends ml.WithEnable {
  @ml.value("string")
  getter: string = "";
  @ml.value("string")
  setter: string = "";
}

class _Gen {
  static body(record: db.Record) {
    const b = record.generate_body_content;

    b.$line(`/*content*/`);
    b.$line(`void* self = nullptr;`);
    b.$line(`const ${record.short_name}_VTable* vtable = nullptr;`);
    b.$line(``);
    b.$line(`/*ctor*/`);
    b.$line(`template<typename T>`);
    b.$line(`${record.short_name}(T& obj) noexcept`);
    b.$indent((_b) => {
      b.$line(`: self(&obj)`);
      b.$line(`, vtable(${record.short_name}_VTableTraits<T>::get_vtable())`);
    });
    b.$line(`{`);
    b.$line(`}`);
  }
  static header(header: db.Header) {
    const b = header.gen_code;

    b.$line(`// BEGIN PROXY GENERATED`);
    b.$line(``);

    const _gen_records = header.records.filter(record => record.ml_configs.proxy.enable);

    // gen concept
    b.$line(`// concepts`);
    _gen_records.forEach((record) => {
      const _gen_methods = record.methods.filter(method => method.ml_configs.proxy.enable);
      const concept_ns = record.namespace.length > 0
        ? `proxy_concept::${record.namespace.join("::")}`
        : `proxy_concept`;

      b.$line(`// concept of ${record.name}`);
      b.$namespace(concept_ns, (b) => {
        _gen_methods.forEach((method) => {
          const method_config = method.ml_configs.proxy as MethodConfig;

          b.$line(`template <typename T>`);
          b.$line(`concept has_${method.short_name} = requires(${method.dump_const()} T t ${method.dump_params_with_comma()}) {`);
          b.$indent((_b) => {
            b.$line(`{ t.${method.short_name}( ${method.dump_params_name_only()} ) } -> std::convertible_to<${method.ret_type}>;`);
          });
          b.$line(`};`);
          b.$line(`template <typename T>`);
          b.$line(`concept has_static_${method.short_name} = requires(${method.dump_const()} T t ${method.dump_params_with_comma()}) {`);
          b.$indent((_b) => {
            b.$line(`{ ${method.short_name}( &t ${method.dump_params_name_only_with_comma()} ) } -> std::convertible_to<${method.ret_type}>;`);
          });
          b.$line(`};`);

          if (method_config.setter.length > 0) {
            b.$line(`template <typename T>`);
            b.$line(`concept has_setter_${method.short_name} = requires(${method.dump_const()} T t ${method.dump_params_with_comma()}) {`);
            b.$indent((_b) => {
              b.$line(`{ t.${method_config.setter} = ${method.dump_params_name_only()} };`);
            });

            b.$line(`};`);
          }
          if (method_config.getter.length > 0) {
            b.$line(`template <typename T>`);
            b.$line(`concept has_getter_${method.short_name} = requires(${method.dump_const()} T t) {`);
            b.$indent((_b) => {
              b.$line(`{ t.${method_config.getter} } -> std::convertible_to<${method.ret_type}>;`);
            });
            b.$line(`};`);
          }
        });
      });
    });
    b.$line(``);

    // gen vtable
    b.$line(`// vtable`);
    _gen_records.forEach((record) => {
      const _gen_methods = record.methods.filter(method => method.ml_configs.proxy.enable);
      const concept_ns = record.namespace.length > 0
        ? `proxy_concept::${record.namespace.join("::")}`
        : `proxy_concept`;

      // gen vtable content
      b.$namespace(record.namespace.join("::"), (b) => {
        // gen vtable content
        b.$struct(`${record.short_name}_VTable`, (b) => {
          _gen_methods.forEach((method) => {
            b.$line(`${method.ret_type} (*${method.short_name})(${method.dump_const()} void* self ${method.dump_params_with_comma()}) = nullptr;`);
          });
        });

        // gen vtable traits
        b.$line("template <typename T>");
        b.$struct(`${record.short_name}_VTableTraits`, (b) => {
          // gen static getter helper
          _gen_methods.forEach((method) => {
            const method_config = method.ml_configs.proxy as MethodConfig;
            const return_expr = method.has_return() ? "return " : "";

            b.$line(`inline static ${method.ret_type} static_${method.short_name}(${method.dump_const()} void* self ${method.dump_params_with_comma()}) ${method.dump_noexcept()} {`);
            b.$indent((_b) => {
              // method
              b.$line(`if constexpr (${concept_ns}::has_${method.short_name}<T>) {`);
              b.$indent((_b) => {
                b.$line(`${return_expr}static_cast<${method.dump_const()} T*>(self)->${method.short_name}(${method.dump_params_name_only()});`);
              });

              // function
              b.$line(`} else if constexpr (${concept_ns}::has_static_${method.short_name}<T>) {`);
              b.$indent((_b) => {
                b.$line(`${return_expr}${method.short_name}(static_cast<${method.dump_const()} T*>(self) ${method.dump_params_name_only_with_comma()});`);
              });

              // field setter
              if (method_config.setter.length > 0) {
                b.$line(`} else if constexpr (${concept_ns}::has_setter_${method.short_name}<T>) {`);
                b.$indent((_b) => {
                  b.$line(`static_assert(std::is_same_v<${method.ret_type}, void>, "Setter must return void");`);
                  b.$line(`${return_expr}static_cast<${method.dump_const()} T*>(self)->${method_config.setter} = ${method.dump_params_name_only()};`);
                });
              }

              // field getter
              if (method_config.getter.length > 0) {
                b.$line(`} else if constexpr (${concept_ns}::has_getter_${method.short_name}<T>) {`);
                b.$indent((_b) => {
                  b.$line(`${return_expr}static_cast<${method.dump_const()} T*>(self)->${method_config.getter};`);
                });
              }

              b.$line(`}`);
            });
            b.$line(`}`);
          });

          // gen vtable getter
          b.$line(`static ${record.short_name}_VTable* get_vtable() {`);
          b.$indent((_b) => {
            // static table
            b.$line(`static ${record.short_name}_VTable _vtable = {`);
            b.$indent((_b) => {
              _gen_methods.forEach((method) => {
                b.$line(`&static_${method.short_name},`);
              });
            });
            b.$line(`};`);

            // return table
            b.$line(`return &_vtable;`);
          });
          b.$line(`}`);
        });
      });
    });
    b.$line(``);

    // gen traits
    b.$line(`// traits`);
    b.$namespace("skr", (_b) => {
      _gen_records.forEach((record) => {
        const _gen_methods = record.methods.filter(method => method.ml_configs.proxy.enable);
        const concept_ns = record.namespace.length > 0
          ? `proxy_concept::${record.namespace.join("::")}`
          : `proxy_concept`;

        b.$line(`template <>`);
        b.$line(`struct ProxyObjectTraits<${record.name}> {`);
        b.$indent((_b) => {
          b.$line(`template <typename T>`);
          b.$line(`static constexpr bool validate() {`);
          b.$indent((_b) => {
            b.$line(`return`);
            _gen_methods.forEach((method) => {
              const method_config = method.ml_configs.proxy as MethodConfig;
              b.$line(`( // ${method.name}`);
              b.$indent((_b) => {
                b.$line(`${concept_ns}::has_${method.short_name}<T>`);
                b.$line(`|| ${concept_ns}::has_static_${method.short_name}<T>`);
                if (method_config.setter.length > 0) {
                  b.$line(
                    `|| ${concept_ns}::has_setter_${method.short_name}<T>`,
                  );
                } else if (method_config.getter.length > 0) {
                  b.$line(
                    `|| ${concept_ns}::has_getter_${method.short_name}<T>`,
                  );
                }
              });
              b.$line(`)`);
              b.$line(`&&`);
            });
            b.$line(`true;`);
          });
          b.$line(`}`);
        });
        b.$line(`};`);
      });
    });

    b.$line(`// END PROXY GENERATED`);
  }
  static source(main_db: db.Module) {
    const b = main_db.source_batch.get_default()
    const _gen_records = main_db.filter_record(record => record.ml_configs.proxy.enable);

    // gen member func impl
    b.$line(`// BEGIN PROXY GENERATED`);
    b.$line(`#include "SkrRTTR/proxy.hpp"`)
    _gen_records.forEach((record) => {
      const _gen_methods = record.methods.filter(method => method.ml_configs.proxy.enable);

      b.$line(`// ${record.name}`);

      // gen func impl
      _gen_methods.forEach(method => {
        b.$line(
          `${method.ret_type} ${method.name}(${method.dump_params()}) ${method.dump_modifiers()}`,
        );
        b.$scope((b) => {
          b.$line(
            `return vtable->${method.short_name}(self ${method.dump_params_name_only_with_comma()});`,
          );
        });
      });
    });
    b.$line(`// END PROXY GENERATED`);
  }
}

class ProxyGenerator extends gen.Generator {
  override inject_configs(): void {
    this.main_module_db.each_record((record) => {
      record.ml_configs.proxy = new RecordConfig();
      record.methods.forEach((method) => {
        method.ml_configs.proxy = new MethodConfig();
      });
    });
  }

  override gen_body(): void {
    this.main_module_db.each_record((record) => {
      if (record.ml_configs.proxy.enable) {
        _Gen.body(record);
      }
    });
  }
  override gen(): void {
    // gen header
    this.main_module_db.headers.forEach((header) => {
      _Gen.header(header);
    });

    // gen source
    _Gen.source(this.main_module_db);
  }
}

export function load_generator(gm: gen.GenerateManager) {
  gm.add_generator("proxy", new ProxyGenerator());
}
