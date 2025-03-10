import * as gen from "@framework/generator"
import * as db from "@framework/database"
import { CodeBuilder } from "@framework/utils"

class _Gen {
  static header_pre(header: db.Header) {
    // combine generate body
    const generate_body_content = new CodeBuilder()
    header.records.forEach((record) => {
      if (record.has_generate_body_flag) {
        generate_body_content.line(`#define SKR_GENERATE_BODY_${header.file_id}_${record.generate_body_line} ${record.dump_generate_body()}`)
      }
    })

    // combine forward declarations
    const forward_declarations = new CodeBuilder()
    header.records.forEach((record) => {
      forward_declarations.namespace_line(record.namespace.join("::"), () => {
        return `struct ${record.short_name};`
      })
    })
    header.enums.forEach((enum_) => {
      let prefix = enum_.is_scoped ? "class" : "";
      let underlying_type = enum_.underlying_type != 'unfixed' ? `: ${enum_.underlying_type}` : "";
      forward_declarations.namespace_line(enum_.namespace.join('::'), () => {
        return `enum ${prefix} ${enum_.short_name}${underlying_type};`
      })
    })

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
${generate_body_content}
// END Generated Body

// BEGIN forward declarations
${forward_declarations}
// END forward declarations
`)
  }
  static source_pre(main_db: db.Module) {
    // combine include headers
    const include_headers = new CodeBuilder()
    main_db.headers.forEach((header) => {
      if (header.header_path.length > 0) {
        include_headers.line(`#include "${header.header_path}"`)
      }
    })

    main_db.gen_code.block(`//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!! THIS FILE IS GENERATED, ANY CHANGES WILL BE LOST !!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// BEGIN header includes
// #define private public
// #define protected public
${include_headers}
// #undef private
// #undef protected
// END header includes

// BEGIN push diagnostic
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimplicitly-unsigned-literal"
#endif
// END push diagnostic
`)
  }
  static source_post(main_db: db.Module) {
    main_db.gen_code.block(`
// BEGIN pop diagnostic
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
// END pop diagnostic`)
  }
}

class BasicGenerator extends gen.Generator {
  pre_gen(): void {
    this.main_module_db.each_record((record, _header) => {
      // check GENERATE_BODY()
      if (!record.generate_body_content.is_empty() && !record.has_generate_body_flag) {
        throw new Error(
          `The record ${record.name} has a body content but the generate_body_flag is not set. Please check the record.`
        )
      }

      // gen header
      this.main_module_db.each_record((record, header) => { _Gen.header_pre(header); })

      // gen source
      _Gen.source_pre(this.main_module_db)
    })
  }
  post_gen(): void {
    // gen source
    _Gen.source_post(this.main_module_db)
  }
}

export function load_generator(gm: gen.GenerateManager) {
  gm.add_generator("basic", new BasicGenerator())
}