import * as gen from "@framework/generator"
import * as db from "@framework/database"
import * as cpp from "@framework/cpp_types"

class ParamData {
  flags: string[] = []
  attrs: string[] = []
}

class MethodData {
  enable: boolean = false
  getter: string = ""
  setter: string = ""
}

class FieldData {
  enable: boolean = false
  getter: string = ""
  setter: string = ""
}

class RecordData {
  enable: boolean = false
  reflect_bases: string[] = []
  exclude_bases: string[] = []
  reflect_fields: boolean = false
  reflect_methods: boolean = false
  flags: string[] = []
  attrs: string[] = []
}

class EnumValueData {
  enable: boolean = false
  flags: string[] = []
  attrs: string[] = []
}

class EnumData {
  enable: boolean = false
  flags: string[] = []
  attrs: string[] = []
}

class RttrGenerator extends gen.Generator {
  override gen_body(): void {
    this.owner.project_db.main_module.each_record((record, _header) => {
      const record_data = record.gen_data.rttr as RecordData
      if (record_data.enable && this.owner.project_db.is_derived(record, "skr::rttr::iobject")) {
        record.generate_body_content += `
::skr::GUID iobject_get_typeid() const override
{
    using namespace skr::rttr;
    using ThisType = std::remove_cv_t<std::remove_pointer_t<decltype(this)>>;
    return type_id_of<ThisType>();
}
void* iobject_get_head_ptr() const override { return const_cast<void*>((const void*)this); }
`
      }
    });
  }
  override gen(): void {
    // gen header
    for (const header_db of this.owner.project_db.main_module.headers) {
      let records_traits = ""
      for (const record of header_db.records) {
        records_traits += `SKR_RTTR_TYPE(${record.name}, "${record.gen_data.guid}")`
      }

      let enums_traits = ""
      for (const enum_obj of header_db.enums) {
        records_traits += `SKR_RTTR_TYPE(${enum_obj.name}, "${enum_obj.gen_data.guid}")`
      }

      this.owner.append_content(
        header_db.output_header_path,
`// BEGIN RTTR GENERATED
#include "SkrRTTR/rttr_traits.hpp"

// rttr traits
${records_traits}
${enums_traits}
// END RTTR GENERATED
`
      )
    }

    // gen source
    this.owner.append_content(
      "generated.cpp",
`// BEGIN RTTR GENERATED
#include "SkrBase/misc/hash.h"
#include "SkrRTTR/type.hpp"
#include "SkrCore/exec_static.hpp"
#include "SkrContainers/tuple.hpp"
#include "SkrRTTR/export/export_builder.hpp"

SKR_EXEC_STATIC_CTOR
{
    using namespace ::skr::rttr;

    //============================> Begin Record Export <============================
${this.#gen_record_rttr(this.owner.project_db.main_module)}
    //============================> End Record Export <============================

    //============================> Begin Enum Export <============================
${this.#gen_enum_rttr(this.owner.project_db.main_module)}
    //============================> End Enum Export <============================
};
// END RTTR GENERATED
`
    )
  }
  #gen_record_rttr(module_db: db.Module) {
    let result = ""
    module_db.each_record((record, _header) => {
      const record_data = record.gen_data.rttr as RecordData
      let reflect_methods: cpp.Method[] = []
      let reflect_fields: cpp.Field[] = []

      let bases_content = ""
      let methods_content = ""
      let fields_content = ""
      let flags_content = ""
      let attrs_content = ""

      result += `
    register_type_loader(type_id_of<${record.name}>(), +[](Type* type) {
        // setup module
        type->set_module(u8"${module_db.config.module_name}");

        // reserve
        record_data->bases_data.reserve(${record.gen_data});
        record_data->methods.reserve(${reflect_methods.length});
        record_data->fields.reserve(${reflect_fields.length});
        
        RecordBuilder<${record.name}> builder(record_data);
        
        // basic
        builder.basic_info();

        // bases
${bases_content}

        // methods
${methods_content}

        // fields
${fields_content}

        // flags
${flags_content}

        // attrs
${attrs_content}

        // build record
        type->build_record([&](RecordData* record_data){
    });
`
    });
    return result
  }
  #gen_enum_rttr(module_db: db.Module) {
    let result = ""
    module_db.each_enum((enum_obj, _header) => {
    })
    return result
  }
}

export function load_generator(gm: gen.GenerateManager) {
  gm.add_generator("rttr", new RttrGenerator())
}