import * as gen from "@framework/generator"
import * as db from "@framework/database"

class _Gen {
  static header_pre(header: db.Header) {
    header.gen_code.block(`//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!! THIS FILE IS GENERATED, ANY CHANGES WILL BE LOST !!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

#pragma once
#include "SkrBase/config.h"
#include <inttypes.h>

#ifdef __meta__
#error "this file should not be inspected by meta"
#endif

#ifdef SKR_FILE_ID
    #undef SKR_FILE_ID
#endif
#define SKR_FILE_ID ${header.file_id}

// BEGIN Generated Body
${_Gen.#generate_body(header)}
// END Generated Body

// BEGIN forward declarations
${_Gen.#fwd_decl(header)}
// END forward declarations
`)
  }

  static #generate_body(header: db.Header): string {
    let result = ""
    for (const record of header.records) {
      if (!record.has_generate_body_flag) continue;
      result += `#define SKR_GENERATE_BODY_${header.file_id}_${record.generate_body_line} ${record.dump_generate_body}\n`
    }
    return result;
  }
  static #fwd_decl(header: db.Header): string {
    let result = ""
    for (const record of header.records) {
      if (record.namespace.length > 0) {
        result += `namespace ${record.namespace.join("::")} { struct ${record.short_name}; }`
      } else {
        result += `struct ${record.short_name};`
      }
    }
    for (const e of header.enums) {
      const prefix = e.is_scoped ? 'enum class' : 'enum';
      const underlying_type = e.underlying_type !== "unfixed" ? `: ${e.underlying_type}` : '';
      if (e.namespace.length > 0) {
        result += `namespace ${e.namespace.join("::")} { ${prefix} ${e.short_name} ${underlying_type}; }`
      } else {
        result += `${prefix} ${e.short_name} ${underlying_type};`
      }
    }
    return result;
  }

  static source_pre(main_module: db.Module) {
    main_module.gen_code.block(`//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!! THIS FILE IS GENERATED, ANY CHANGES WILL BE LOST !!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// BEGIN header includes
${_Gen.#header_includes(main_module)}
// END header includes

// BEGIN push diagnostic
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimplicitly-unsigned-literal"
#endif
// END push diagnostic
`)
  }
  static #header_includes(module: db.Module): string {
    let result = ""
    for (const header of module.headers) {
      if (header.header_path.length > 0) {
        result += `#include "${header.header_path}"\n`
      }
    }
    return result;
  }

  static source_post(main_module: db.Module) {
    main_module.gen_code.block(`// BEGIN pop diagnostic
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
// END pop diagnostic
`);
  }
}


class BasicGenerator extends gen.Generator {
  override pre_gen(): void {
    // test generate body
    this.owner.project_db.main_module.each_record((record, _header) => {
      if (record.generate_body_content.length > 0 && !record.has_generate_body_flag) {
        throw new Error(
          `Record ${record.name} has generate body but not set generate_body_flag`
        )
      }
    });

    // gen header
    for (const header of this.owner.project_db.main_module.headers) {
      _Gen.header_pre(header);
    }

    // gen source
    _Gen.source_pre(this.owner.project_db.main_module);
  }
  override post_gen(): void {
    _Gen.source_post(this.owner.project_db.main_module);
  };


}

export function load_generator(gm: gen.GenerateManager) {
  gm.add_generator("basic", new BasicGenerator())
}