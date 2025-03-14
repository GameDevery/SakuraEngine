import * as gen from "@framework/generator";
import * as db from "@framework/database";
import * as ml from "@framework/meta_lang";
import { CodeBuilder } from "@framework/utils";

class ConfigBase extends ml.WithEnable {
  @ml.array("string")
  flags: string[] = [];

  @ml.array("string")
  attrs: string[] = [];
}

class GuidConfig {
  @ml.value("string")
  value: string = "";

  @ml.value_proxy("string")
  proxy_value(v: string) {
    this.value = v;
  }

  is_empty() { return this.value.length === 0; }

  as_constant() {
    let result = ""

    // 32bit
    result += `0x${this.value.slice(0, 8)}, `   // 1st comp

    // 16bit * 2
    result += `0x${this.value.slice(9, 13)}, `  // 2st comp
    result += `0x${this.value.slice(14, 18)}, ` // 3st comp

    // 8bit * 8
    result += "{"
    result += `0x${this.value.slice(19, 21)}, ` // 4st comp
    result += `0x${this.value.slice(21, 23)}, ` // 4st comp
    result += `0x${this.value.slice(24, 26)}, ` // 5st comp
    result += `0x${this.value.slice(26, 28)}, ` // 5st comp
    result += `0x${this.value.slice(28, 30)}, ` // 5st comp
    result += `0x${this.value.slice(30, 32)}, ` // 5st comp
    result += `0x${this.value.slice(32, 34)}, ` // 5st comp
    result += `0x${this.value.slice(34, 36)}`   // 5st comp
    result += "}"


    return result
  }

  toString() {
    return this.value;
  }
}
type CppConfigTypes =
  | RecordConfig
  | FieldConfig
  | MethodConfig
  | ParamConfig
  | EnumConfig
  | EnumValueConfig;
class RecordConfig extends ConfigBase {
  // default is true
  override enable: boolean = true;

  @ml.value("boolean")
  reflect_bases: boolean = true;

  @ml.array("string")
  exclude_bases: string[] = [];

  @ml.value("boolean")
  reflect_fields(v: boolean) {
    this.#record.fields.forEach(field => {
      field.ml_configs.rttr.enable = v;
    })
  }

  @ml.value("boolean")
  reflect_methods(v: boolean) {
    this.#record.methods.forEach(method => {
      method.ml_configs.rttr.enable = v;
    })
  }

  @ml.preset("full")
  preset_full() {
    this.reflect_bases = true;
    this.reflect_fields(true);
    this.reflect_methods(true);
  }

  @ml.preset("fields")
  preset_fields() {
    this.reflect_fields(true);
  }

  @ml.preset("methods")
  preset_methods() {
    this.reflect_methods(true);
  }

  @ml.preset("minimal")
  preset_minimal() {
    this.reflect_bases = false;
    this.reflect_fields(false);
    this.reflect_methods(false);
  }

  #record: db.Record;
  constructor(record: db.Record) {
    super();
    this.#record = record;
  }
}
class FieldConfig extends ConfigBase {
}
class MethodConfig extends ConfigBase {
}
class ParamConfig extends ConfigBase {
}
class EnumConfig extends ConfigBase {
  override enable: boolean = true;
}
class EnumValueConfig extends ConfigBase {
}

// TODO. 通过 GUID 判断 RTTR 的开关
class _Gen {
  static body(record: db.Record) {
    const b = record.generate_body_content;
    b.$line(``);
    b.$line(`::skr::GUID iobject_get_typeid() const override`);
    b.$scope((_b) => {
      b.$line(`using namespace ::skr;`);
      b.$line(
        `using ThisType = std::remove_cv_t<std::remove_pointer_t<decltype(this)>>;`,
      );
      b.$line(`return type_id_of<ThisType>();`);
    });
    b.$line(
      `void* iobject_get_head_ptr() const override { return const_cast<void*>((const void*)this); }`,
    );
    b.$line(``);
  }
  static header(header: db.Header) {
    const b = header.gen_code;
    const _gen_records = header.records.filter(record => record.ml_configs.rttr.enable && !record.ml_configs.guid.is_empty());
    const _gen_enums = header.enums.filter(enum_ => enum_.ml_configs.rttr.enable && !enum_.ml_configs.guid.is_empty());

    b.$line(`// BEGIN RTTR GENERATED`);
    b.$line(`#include "SkrRTTR/rttr_traits.hpp"`);
    _gen_records.forEach((record) => {
      b.$line(`SKR_RTTR_TYPE(${record.name}, "${record.ml_configs.guid}")`,);
    });
    _gen_enums.forEach((enum_) => {
      b.$line(`SKR_RTTR_TYPE(${enum_.name}, "${enum_.ml_configs.guid}")`,);
    });
    b.$line(`// END RTTR GENERATED`);
  }
  static source(main_db: db.Module) {
    const b = main_db.gen_code;
    const _gen_records = main_db.filter_record(record => record.ml_configs.rttr.enable && !record.ml_configs.guid.is_empty());
    const _gen_enums = main_db.filter_enum(enum_ => enum_.ml_configs.rttr.enable && !enum_.ml_configs.guid.is_empty());

    // header
    b.$line(`// BEGIN RTTR GENERATED`);
    b.$line(`#include "SkrBase/misc/hash.h"`);
    b.$line(`#include "SkrRTTR/type.hpp"`);
    b.$line(`#include "SkrCore/exec_static.hpp"`);
    b.$line(`#include "SkrContainers/tuple.hpp"`);
    b.$line(`#include "SkrRTTR/export/export_builder.hpp"`);
    b.$line(``);

    // static register
    b.$line(`SKR_EXEC_STATIC_CTOR`);
    b.$line(`{`);
    b.$indent(() => {
      b.$line(`using namespace ::skr;`);
      b.$line(``);

      // export records
      _gen_records.forEach((record, _header) => {
        const record_config = record.ml_configs.rttr as RecordConfig;

        // collect export data
        const _gen_bases: string[] = record_config.reflect_bases ?
          record.bases.filter((base) => !record_config.exclude_bases.includes(base)) :
          [];
        const _gen_fields: db.Field[] = record.fields.filter((field) => field.ml_configs.rttr.enable);
        const _gen_methods: db.Method[] = record.methods.filter((method) => method.ml_configs.rttr.enable);

        // register function
        b.$line(`register_type_loader(type_id_of<${record.name}>(), +[](RTTRType* type) {`);
        b.$indent((_b) => {
          // module info
          b.$line(`// setup module`);
          b.$line(`type->set_module(u8"${main_db.config.module_name}");`);
          b.$line(``);

          // build scope
          b.$line(`// build scope`);
          b.$line(`type->build_record([&](RTTRRecordData* record_data) {`);
          b.$indent((_b) => {
            // basic info
            b.$line(`// reserve`);
            b.$line(`record_data->bases_data.reserve(${_gen_bases.length});`);
            b.$line(`record_data->fields.reserve(${_gen_fields.length});`);
            b.$line(`record_data->methods.reserve(${_gen_methods.length});`);
            b.$line(``);
            b.$line(`RTTRRecordBuilder<${record.name}> builder(record_data);`);
            b.$line(``);
            b.$line(`// basic`);
            b.$line(`builder.basic_info();`);
            b.$line(``);

            // bases
            b.$line(`// bases`);
            if (_gen_bases.length > 0) {
              b.$line(`builder.bases<${_gen_bases.join(", ")}>();`);
            }

            // fields
            b.$line(`// fields`);
            _gen_fields.forEach((field) => {
              const field_config = field.ml_configs.rttr as FieldConfig;
              b.$line(`{ // ${record.name}::${field.short_name}`);
              b.$indent((_b) => {
                if (field.is_static) {
                  b.$line(`[[maybe_unused]] auto field_builder = builder.static_field<&${record.name}::${field.short_name}>(u8"${field.short_name}");`,);
                } else {
                  b.$line(`[[maybe_unused]] auto field_builder = builder.field<&${record.name}::${field.short_name}>(u8"${field.short_name}");`,);
                }

                this.#flags_and_attrs(b, field, field_config, "field_builder");
              });
              b.$line(`}`);
            });

            // methods
            b.$line(`// methods`);
            _gen_methods.forEach((method) => {
              const method_config = method.ml_configs.rttr as MethodConfig;
              b.$line(`{ // ${record.name}::${method.name}`);
              b.$indent((_b) => {
                if (method.is_static) {
                  b.$line(`[[maybe_unused]] auto method_builder = builder.static_method<${method.signature()}, &${record.name}::${method.short_name}>(u8"${method.short_name}");`,);
                } else {
                  b.$line(`[[maybe_unused]] auto method_builder = builder.method<${method.signature()}, &${record.name}::${method.short_name}>(u8"${method.short_name}");`,);
                }

                // build params
                b.$line(`// params`);
                method.parameters.forEach((param, idx) => {
                  b.$scope((_b) => {
                    const param_config = param.ml_configs.rttr as ParamConfig;
                    b.$line(`[[maybe_unused]] auto param_builder = method_builder.param_at(${idx});`,);
                    b.$line(`param_builder.name(u8"${param.name}");`);
                    b.$line(``);
                    this.#flags_and_attrs(
                      b,
                      param,
                      param_config,
                      "param_builder",
                    );
                  });
                });

                this.#flags_and_attrs(
                  b,
                  method,
                  method_config,
                  "method_builder",
                );
              });
              b.$line(`}`);
            });

            // flags & attrs
            this.#flags_and_attrs(b, record, record_config, "builder");
          });
          b.$line(`});`);
        });
        b.$line(`});`);
      });

      // export enums
      _gen_enums.forEach((enum_) => {
        const enum_config = enum_.ml_configs.rttr as EnumConfig;
        const _gen_enum_values = enum_.values.filter((enum_value) => enum_value.ml_configs.rttr.enable);

        // register function
        b.$line(`register_type_loader(type_id_of<${enum_.name}> (), +[](RTTRType * type){`,);
        b.$indent((_b) => {
          // module info
          b.$line(`// setup module`);
          b.$line(`type->set_module(u8"${main_db.config.module_name}");`);
          b.$line(``);

          // build scope
          b.$line(`// build scope`);
          b.$line(`type->build_enum([&](RTTREnumData* enum_data) {`);
          b.$indent((_b) => {
            // basic
            b.$line(`// reserve`);
            b.$line(`enum_data->items.reserve(${enum_.values.length});`);
            b.$line(``);
            b.$line(`RTTREnumBuilder<${enum_.name}> builder(enum_data);`);
            b.$line(``);
            b.$line(`// basic`);
            b.$line(`builder.basic_info();`);
            b.$line(``);

            // items
            b.$line(`// items`);
            _gen_enum_values.forEach((enum_value) => {
              const enum_value_config = enum_value.ml_configs.rttr as EnumValueConfig;

              b.$line(`{ // ${enum_.name}::${enum_value.name}`);
              b.$indent((_b) => {
                b.$line(`[[maybe_unused]] auto item_builder = builder.item(u8"${enum_value.short_name}", ${enum_value.name});`,);
                b.$line(``);
                this.#flags_and_attrs(
                  b,
                  enum_value,
                  enum_value_config,
                  "item_builder",
                );
              });
              b.$line(`}`);
            });

            this.#flags_and_attrs(b, enum_, enum_config, "builder");
          });
          b.$line(`});`);
        });
        b.$line(`});`);
      });
    });
    b.$line(`};`);

    // bottom
    b.$line(`// END RTTR GENERATED`);
    b.$line(``);
  }
  static #flags_and_attrs(
    b: CodeBuilder,
    cpp_type: db.CppTypes,
    config: CppConfigTypes,
    builder_name: string,
  ) {
    b.$line(`// flags`);
    if (config.flags.length > 0) {
      b.$line(`${builder_name}.flag(${this.#flags_expr(cpp_type, config.flags)});`,);
    }

    b.$line(`// attrs`);
    if (config.attrs.length > 0) {
      b.$line(`${builder_name}`);
      b.$indent((_b) => {
        config.attrs.forEach((attr) => {
          b.$line(`.attribute(::skr::attr::${attr})`);
        });
      });
      b.$line(`;`);
    }
  }
  static #flag_enum_name_of(cpp_type: db.CppTypes) {
    if (cpp_type instanceof db.Record) {
      return "ERTTRRecordFlag";
    } else if (cpp_type instanceof db.Field) {
      return cpp_type.is_static ? "ERTTRStaticFieldFlag" : "ERTTRFieldFlag";
    } else if (cpp_type instanceof db.Method) {
      return cpp_type.is_static ? "ERTTRStaticMethodFlag" : "ERTTRMethodFlag";
    } else if (cpp_type instanceof db.Parameter) {
      return "ERTTRParamFlag";
    } else if (cpp_type instanceof db.Enum) {
      return "ERTTREnumFlag";
    } else if (cpp_type instanceof db.EnumValue) {
      return "ERTTREnumItemFlag";
    } else {
      throw new Error(`Unknown cpp_type: ${cpp_type.constructor.name}`);
    }
  }
  static #flags_expr(cpp_type: db.CppTypes, flags: string[]) {
    return flags.map((flags) =>
      `${this.#flag_enum_name_of(cpp_type)}::${flags}`
    ).join(" | ");
  }
}

class RttrGenerator extends gen.Generator {
  override inject_configs(): void {
    // record
    this.main_module_db.each_record((record) => {
      record.ml_configs.rttr = new RecordConfig(record);
      record.ml_configs.guid = new GuidConfig();

      // methods
      record.methods.forEach((method) => {
        method.ml_configs.rttr = new MethodConfig();

        // params
        method.parameters.forEach((param) => {
          param.ml_configs.rttr = new ParamConfig();
        });
      });

      // fields
      record.fields.forEach((field) => {
        field.ml_configs.rttr = new FieldConfig();
      });
    });

    // enums
    this.main_module_db.each_enum((enum_) => {
      enum_.ml_configs.rttr = new EnumConfig();
      enum_.ml_configs.guid = new GuidConfig();
      enum_.values.forEach((enum_value) => {
        enum_value.ml_configs.rttr = new EnumValueConfig();
      });
    });
  }

  gen_body(): void {
    this.main_module_db.each_record((record, header) => {
      if (this.project_db.is_derived(record, "skr::IObject")) {
        _Gen.body(record);
      }
    });
  }
  gen(): void {
    // gen headers
    this.main_module_db.headers.forEach((header) => {
      _Gen.header(header);
    });

    // gen source
    _Gen.source(this.main_module_db);
  }
}

export function load_generator(gm: gen.GenerateManager) {
  gm.add_generator("rttr", new RttrGenerator());
}
