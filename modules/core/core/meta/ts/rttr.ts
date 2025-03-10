import * as gen from "@framework/generator"
import * as db from "@framework/database"
import * as ml from "@framework/meta_lang"
import { CodeBuilder } from "@framework/utils"

class ConfigBase extends ml.WithEnable {
  @ml.array('string')
  flags: string[] = []

  @ml.array('string')
  attrs: string[] = []
}

class GuidConfig {
  @ml.value('string')
  value: string = ""

  @ml.value_proxy('string')
  proxy_value(v: string) {
    this.value = v;
  }

  toString() { return this.value; }
}
type CppConfigTypes =
  RecordConfig |
  FieldConfig |
  MethodConfig |
  ParamConfig |
  EnumConfig |
  EnumValueConfig;
class RecordConfig extends ConfigBase {
  // default is true 
  override enable: boolean = true

  @ml.value('boolean')
  reflect_bases: boolean = true

  @ml.array('string')
  exclude_bases: string[] = []

  @ml.value('boolean')
  reflect_fields: boolean = false

  @ml.value('boolean')
  reflect_methods: boolean = false

  @ml.preset('full')
  preset_full() {
    this.reflect_bases = true;
    this.reflect_fields = true;
    this.reflect_methods = true;
  }

  @ml.preset('fields')
  preset_fields() {
    this.reflect_fields = true;
  }

  @ml.preset('methods')
  preset_methods() {
    this.reflect_methods = true;
  }

  @ml.preset('minimal')
  preset_minimal() {
    this.reflect_bases = false;
    this.reflect_fields = false;
    this.reflect_methods = false;
  }
}
class FieldConfig extends ConfigBase {
}
class MethodConfig extends ConfigBase {
}
class ParamConfig extends ConfigBase {
}
class EnumConfig extends ConfigBase {
}
class EnumValueConfig extends ConfigBase {
}

class _Gen {
  static body(record: db.Record) {
    const b = record.generate_body_content
    b.line(``)
    b.line(`::skr::GUID iobject_get_typeid() const override`)
    b.line(`{`)
    b.line(`    using namespace skr::rttr;`)
    b.line(`    using ThisType = std::remove_cv_t<std::remove_pointer_t<decltype(this)>>;`)
    b.line(`    return type_id_of<ThisType>();`)
    b.line(`}`)
    b.line(`void* iobject_get_head_ptr() const override { return const_cast<void*>((const void*)this); }`)
    b.line(``)
  }
  static header(header: db.Header) {
    const b = header.gen_code

    const rttr_traits = new CodeBuilder();
    header.each_record((record) => {
      const record_config = record.gen_data.rttr as RecordConfig;
      if (!record_config.enable) return;
      rttr_traits.line(`SKR_RTTR_TYPE(${record.name}, "${record.gen_data.guid}")`)
    })
    header.each_enum((enum_) => {
      const enum_config = enum_.gen_data.rttr as EnumConfig;
      if (!enum_config.enable) return;
      rttr_traits.line(`SKR_RTTR_TYPE(${enum_.name}, "${enum_.gen_data.guid}")`)
    })

    b.line(`// BEGIN RTTR GENERATED`)
    b.line(`#include "SkrRTTR/rttr_traits.hpp"`)
    b.line(`${rttr_traits}`)
    b.line(`// END RTTR GENERATED`)
  }
  static source(main_db: db.Module) {
    const b = main_db.gen_code;

    b.line(`// BEGIN RTTR GENERATED`)
    b.line(`#include "SkrBase/misc/hash.h"`)
    b.line(`#include "SkrRTTR/type.hpp"`)
    b.line(`#include "SkrCore/exec_static.hpp"`)
    b.line(`#include "SkrContainers/tuple.hpp"`)
    b.line(`#include "SkrRTTR/export/export_builder.hpp"`)
    b.line(``)
    b.line(`SKR_EXEC_STATIC_CTOR`)
    b.line(`{`)
    b.line(`    using namespace ::skr::rttr;`)
    b.line(``)
    b.push_indent();

    // export records
    main_db.each_record((record, _header) => {
      const record_config = record.gen_data.rttr as RecordConfig;
      if (!record_config.enable) return;

      // collect export data
      const bases: string[] = record.bases.filter(base => !record_config.exclude_bases.includes(base))
      const fields: db.Field[] = record.fields.filter(field => field.gen_data.rttr.enable)
      const methods: db.Method[] = record.methods.filter(method => method.gen_data.rttr.enable)

      // register function
      b.scope(`register_type_loader(type_id_of<${record.name}>(), +[](Type* type) {`, `});`, _b => {
        // module info
        b.line(`// setup module`)
        b.line(`type->set_module(u8"${main_db.config.module_name}");`)
        b.line(``);

        // build scope
        b.line(`// build scope`)
        b.scope(`type->build_record([&](RecordData* record_data) {`, `});`, _b => {
          // basic info
          b.line(`// reserve`)
          b.line(`record_data->bases_data.reserve(${bases.length});`)
          b.line(`record_data->fields.reserve(${fields.length});`)
          b.line(`record_data->methods.reserve(${methods.length});`)
          b.line(``)
          b.line(`RecordBuilder<${record.name}> builder(record_data);`)
          b.line(``)
          b.line(`// basic`)
          b.line(`builder.basic_info();`)
          b.line(``)

          // bases
          b.line(`// bases`)
          if (bases.length > 0) {
            b.line(`builder.bases<${bases.join(", ")}>();`)
          }

          // fields
          b.line(`// fields`)
          fields.forEach(field => {
            const field_config = field.gen_data.rttr as FieldConfig;
            b.scope(`{ // ${record.name}::${field.name}`, `}`, _b => {
              if (field.is_static) {
                b.line(`[[maybe_unused]] auto field_builder = builder.static_field<&${record.name}::${field.name}>(u8"${field.name}");`)
              } else {
                b.line(`[[maybe_unused]] auto field_builder = builder.field<&${record.name}::${field.name}>(u8"${field.name}");`);
              }

              this.#flags_and_attrs(b, field, field_config, "field_builder")
            })
          })

          // methods
          b.line(`// methods`)
          methods.forEach(method => {
            const method_config = method.gen_data.rttr as MethodConfig;
            b.scope(`{ // ${record.name}::${method.name}`, `}`, _b => {
              if (method.is_static) {
                b.line(`[[maybe_unused]] auto method_builder = builder.static_method<${method.signature()}, &${record.name}::${method.short_name}>(u8"${method.short_name}");`)
              } else {
                b.line(`[[maybe_unused]] auto method_builder = builder.method<${method.signature()}, &${record.name}::${method.short_name}>(u8"${method.short_name}");`)
              }

              b.line(`// params`)
              method.parameters.forEach((param, idx) => {
                const param_config = param.gen_data.rttr as ParamConfig;
                b.line(`method_builder.param_at(${idx})`)
                b.line(`    .name(u8"${param.name}");`)
                b.line(``)
                // TODO. params flags & attrs
              })

              this.#flags_and_attrs(b, method, method_config, "method_builder")
            })
          })

          // flags & attrs
          this.#flags_and_attrs(b, record, record_config, "builder")
        })
      })
    })

    // export enums
    main_db.each_enum((enum_, _header) => {
      const enum_config = enum_.gen_data.rttr as EnumConfig;
      if (!enum_config.enable) return;

      main_db.each_enum(enum_ => {
        const enum_config = enum_.gen_data.rttr as EnumConfig;
        if (!enum_config.enable) return;

        // register function
        b.scope(`register_type_loader(type_id_of<${enum_.name}> (), +[](Type * type){`, `});`, _b => {
          // module info
          b.line(`// setup module`)
          b.line(`type->set_module(u8"${main_db.config.module_name}");`)
          b.line(``);

          // build scope
          b.line(`// build scope`)
          b.scope(`type->build_enum([&](EnumData* enum_data) {`, `});`, _b => {
            // basic
            b.line(`// reserve`)
            b.line(`enum_data->items.reserve(${enum_.values.length});`)
            b.line(``);
            b.line(`EnumBuilder<${enum_.name}> builder(enum_data);`)
            b.line(``);
            b.line(`// basic`)
            b.line(`builder.basic_info();`)
            b.line(``);

            // items
            b.line(`// items`)
            enum_.values.forEach(enum_value => {
              const enum_value_config = enum_value.gen_data.rttr as EnumValueConfig;
              b.scope(`{ // ${enum_.name}::${enum_value.name}`, `}`, _b => {
                b.line(`[[maybe_unused]] auto item_builder = builder.item(u8"${enum_value.short_name}", ${enum_value.name});`)
                b.line(``);
                this.#flags_and_attrs(b, enum_value, enum_value_config, "item_builder")
              })
            })

            this.#flags_and_attrs(b, enum_, enum_config, "builder")
          });
        })
      })
    })

    b.pop_indent();
    b.line(``)
    b.line(`};`)
    b.line(`// END RTTR GENERATED`)
    b.line(``)
  }
  static #flags_and_attrs(builder: CodeBuilder, cpp_type: db.CppTypes, config: CppConfigTypes, builder_name: string) {
    builder.line(`// flags`)
    if (config.flags.length > 0) {
      builder.line(`${builder_name}.flag(${this.#flags_expr(cpp_type, config.flags)});`)
    }

    builder.line(`// attrs`)
    builder.line(`${builder_name}`)
    config.attrs.forEach(attr => {
      builder.line(`    .attribute(::skr::attr::${attr})`)
    })
    builder.line(`;`)
  }
  static #flag_enum_name_of(cpp_type: db.CppTypes) {
    if (cpp_type instanceof db.Record) {
      return "ERecordFlag"
    } else if (cpp_type instanceof db.Field) {
      return cpp_type.is_static ? "EStaticFieldFlag" : "EFieldFlag"
    } else if (cpp_type instanceof db.Method) {
      return cpp_type.is_static ? "EStaticMethodFlag" : "EMethodFlag"
    } else if (cpp_type instanceof db.Parameter) {
      return "EParamFlag"
    } else if (cpp_type instanceof db.Enum) {
      return "EEnumFlag"
    } else if (cpp_type instanceof db.EnumValue) {
      return "EEnumItemFlag"
    } else {
      throw new Error(`Unknown cpp_type: ${cpp_type.constructor.name}`);
    }
  }
  static #flags_expr(cpp_type: db.CppTypes, flags: string[]) {
    return flags.map(flags => `${this.#flag_enum_name_of(cpp_type)}::${flags}`).join(" | ")
  }
}

class RttrGenerator extends gen.Generator {
  gen_body(): void {
    this.main_module_db.each_record((record, header) => {
      _Gen.body(record);
    });
  }
  gen(): void {
    // gen headers
    this.main_module_db.headers.forEach(header => {
      _Gen.header(header);
    })

    // gen source
    _Gen.source(this.main_module_db);
  }
}

export function load_generator(gm: gen.GenerateManager) {
  gm.add_generator("rttr", new RttrGenerator())
}