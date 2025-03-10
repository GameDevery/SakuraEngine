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
    b.$line(``)
    b.$line(`::skr::GUID iobject_get_typeid() const override`)
    b.$scope(_b => {
      b.$line(`using namespace skr::rttr;`)
      b.$line(`using ThisType = std::remove_cv_t<std::remove_pointer_t<decltype(this)>>;`)
      b.$line(`return type_id_of<ThisType>();`)
    })
    b.$line(`void* iobject_get_head_ptr() const override { return const_cast<void*>((const void*)this); }`)
    b.$line(``)
  }
  static header(header: db.Header) {
    const b = header.gen_code

    const rttr_traits = new CodeBuilder();
    header.each_record((record) => {
      const record_config = record.gen_data.rttr as RecordConfig;
      if (!record_config.enable) return;
      rttr_traits.$line(`SKR_RTTR_TYPE(${record.name}, "${record.gen_data.guid}")`)
    })
    header.each_enum((enum_) => {
      const enum_config = enum_.gen_data.rttr as EnumConfig;
      if (!enum_config.enable) return;
      rttr_traits.$line(`SKR_RTTR_TYPE(${enum_.name}, "${enum_.gen_data.guid}")`)
    })

    b.$line(`// BEGIN RTTR GENERATED`)
    b.$line(`#include "SkrRTTR/rttr_traits.hpp"`)
    b.$line(`${rttr_traits}`)
    b.$line(`// END RTTR GENERATED`)
  }
  static source(main_db: db.Module) {
    const b = main_db.gen_code;

    // header
    b.$line(`// BEGIN RTTR GENERATED`)
    b.$line(`#include "SkrBase/misc/hash.h"`)
    b.$line(`#include "SkrRTTR/type.hpp"`)
    b.$line(`#include "SkrCore/exec_static.hpp"`)
    b.$line(`#include "SkrContainers/tuple.hpp"`)
    b.$line(`#include "SkrRTTR/export/export_builder.hpp"`)
    b.$line(``)

    // static register
    b.$line(`SKR_EXEC_STATIC_CTOR`)
    b.$line(`{`)
    b.$indent(() => {
      b.$line(`using namespace ::skr::rttr;`)
      b.$line(``)

      // export records
      main_db.each_record((record, _header) => {
        const record_config = record.gen_data.rttr as RecordConfig;
        if (!record_config.enable) return;

        // collect export data
        const bases: string[] = record.bases.filter(base => !record_config.exclude_bases.includes(base))
        const fields: db.Field[] = record.fields.filter(field => field.gen_data.rttr.enable)
        const methods: db.Method[] = record.methods.filter(method => method.gen_data.rttr.enable)

        // register function
        b.$line(`register_type_loader(type_id_of<${record.name}>(), +[](Type* type) {`)
        b.$indent(_b => {
          // module info
          b.$line(`// setup module`)
          b.$line(`type->set_module(u8"${main_db.config.module_name}");`)
          b.$line(``);

          // build scope
          b.$line(`// build scope`)
          b.$line(`type->build_record([&](RecordData* record_data) {`)
          b.$indent(_b => {
            // basic info
            b.$line(`// reserve`)
            b.$line(`record_data->bases_data.reserve(${bases.length});`)
            b.$line(`record_data->fields.reserve(${fields.length});`)
            b.$line(`record_data->methods.reserve(${methods.length});`)
            b.$line(``)
            b.$line(`RecordBuilder<${record.name}> builder(record_data);`)
            b.$line(``)
            b.$line(`// basic`)
            b.$line(`builder.basic_info();`)
            b.$line(``)

            // bases
            b.$line(`// bases`)
            if (bases.length > 0) {
              b.$line(`builder.bases<${bases.join(", ")}>();`)
            }

            // fields
            b.$line(`// fields`)
            fields.forEach(field => {
              const field_config = field.gen_data.rttr as FieldConfig;
              b.$line(`{ // ${record.name}::${field.name}`)
              b.$indent(_b => {
                if (field.is_static) {
                  b.$line(`[[maybe_unused]] auto field_builder = builder.static_field<&${record.name}::${field.name}>(u8"${field.name}");`)
                } else {
                  b.$line(`[[maybe_unused]] auto field_builder = builder.field<&${record.name}::${field.name}>(u8"${field.name}");`);
                }

                this.#flags_and_attrs(b, field, field_config, "field_builder")
              })
              b.$line(`}`)
            })

            // methods
            b.$line(`// methods`)
            methods.forEach(method => {
              const method_config = method.gen_data.rttr as MethodConfig;
              b.$line(`{ // ${record.name}::${method.name}`)
              b.$indent(_b => {
                if (method.is_static) {
                  b.$line(`[[maybe_unused]] auto method_builder = builder.static_method<${method.signature()}, &${record.name}::${method.short_name}>(u8"${method.short_name}");`)
                } else {
                  b.$line(`[[maybe_unused]] auto method_builder = builder.method<${method.signature()}, &${record.name}::${method.short_name}>(u8"${method.short_name}");`)
                }

                // build params
                b.$line(`// params`)
                method.parameters.forEach((param, idx) => {
                  b.$scope(_b => {
                    const param_config = param.gen_data.rttr as ParamConfig;
                    b.$line(`[[maybe_unused]] auto param_builder = method_builder.param_at(${idx});`)
                    b.$line(`param_builder.name(u8"${param.name}");`)
                    b.$line(``)
                    this.#flags_and_attrs(b, param, param_config, "param_builder")
                  })
                })

                this.#flags_and_attrs(b, method, method_config, "method_builder")
              })
              b.$line(`}`)
            })

            // flags & attrs
            this.#flags_and_attrs(b, record, record_config, "builder")
          })
          b.$line(`});`)
        })
        b.$line(`});`)
      })

      // export enums
      main_db.each_enum((enum_, _header) => {
        const enum_config = enum_.gen_data.rttr as EnumConfig;
        if (!enum_config.enable) return;

        main_db.each_enum(enum_ => {
          const enum_config = enum_.gen_data.rttr as EnumConfig;
          if (!enum_config.enable) return;

          // register function
          b.$line(`register_type_loader(type_id_of<${enum_.name}> (), +[](Type * type){`)
          b.$indent(_b => {
            // module info
            b.$line(`// setup module`)
            b.$line(`type->set_module(u8"${main_db.config.module_name}");`)
            b.$line(``);

            // build scope
            b.$line(`// build scope`)
            b.$line(`type->build_enum([&](EnumData* enum_data) {`)
            b.$indent(_b => {
              // basic
              b.$line(`// reserve`)
              b.$line(`enum_data->items.reserve(${enum_.values.length});`)
              b.$line(``);
              b.$line(`EnumBuilder<${enum_.name}> builder(enum_data);`)
              b.$line(``);
              b.$line(`// basic`)
              b.$line(`builder.basic_info();`)
              b.$line(``);

              // items
              b.$line(`// items`)
              enum_.values.forEach(enum_value => {
                const enum_value_config = enum_value.gen_data.rttr as EnumValueConfig;
                b.$line(`{ // ${enum_.name}::${enum_value.name}`)
                b.$indent(_b => {
                  b.$line(`[[maybe_unused]] auto item_builder = builder.item(u8"${enum_value.short_name}", ${enum_value.name});`)
                  b.$line(``);
                  this.#flags_and_attrs(b, enum_value, enum_value_config, "item_builder")
                })
                b.$line(`}`)
              })

              this.#flags_and_attrs(b, enum_, enum_config, "builder")
            });
            b.$line(`});`)
          })
          b.$line(`});`)
        })
      })
    })
    b.$line(`};`)

    // bottom
    b.$line(`// END RTTR GENERATED`)
    b.$line(``)
  }
  static #flags_and_attrs(b: CodeBuilder, cpp_type: db.CppTypes, config: CppConfigTypes, builder_name: string) {
    b.$line(`// flags`)
    if (config.flags.length > 0) {
      b.$line(`${builder_name}.flag(${this.#flags_expr(cpp_type, config.flags)});`)
    }

    b.$line(`// attrs`)
    b.$line(`${builder_name}`)
    b.$indent(_b => {
      config.attrs.forEach(attr => {
        b.$line(`.attribute(::skr::attr::${attr})`)
      })
    })
    b.$line(`;`)
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