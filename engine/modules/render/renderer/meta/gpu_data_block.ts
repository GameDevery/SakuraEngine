import * as gen from "@framework/generator";
import * as db from "@framework/database";
import * as ml from "@framework/meta_lang";
import { CodeBuilder } from "@framework/utils";

class SOAConfig extends ml.WithEnable {
  @ml.value("number")
  page_size: number = 0;
}
class AOSConfig extends ml.WithEnable {
}

class RecordConfig extends ml.WithEnable {
  soa: SOAConfig = new SOAConfig();
  aos: AOSConfig = new AOSConfig();
}

class _Gen {
  static header_data_blocks(header: db.Header) {
    if (!header.gen_code_custom.datablocks) {
      header.gen_code_custom.datablocks = new CodeBuilder();
      const b = header.gen_code_custom.datablocks;
      b.$generate_note()
    }
    const b = header.gen_code_custom.datablocks;
    const _gen_records = header.records.filter(_Gen.filter_record);

    // check records
    for (const record of _gen_records) {
      const gpu_cfg = record.ml_configs.gpu as RecordConfig;
      if (gpu_cfg.soa.enable && gpu_cfg.aos.enable) {
        throw new Error(`record ${record.name} cannot enable both soa and aos`);
      }
    }

    // filter soa & aos records
    const _gen_soa_records = _gen_records.filter(r => (r.ml_configs.gpu as RecordConfig).soa.enable);
    const _gen_aos_records = _gen_records.filter(r => (r.ml_configs.gpu as RecordConfig).aos.enable);

    // header basic
    b.$line("#pragma once")

    // include files
    b.$line(`#include <${header.header_path}>`)
    b.$line(``)

    // gen datablocks
    if (_gen_aos_records.length > 0) {
      b.$line(`//! BEGIN Data Blocks`)
      _gen_aos_records.forEach((record) => {
        b.$line(`template <>`)
        b.$line(`struct GPUDatablock<${record.name}> {`)
        b.$indent(_b => {
          // build size traits
          const size_expr = record.fields
            .map(f => `GPUDatablock<${f.type}>::Size`)
            .join("");
          b.$line(`inline static constexpr uint32_t Size =`)
          b.$indent(_b => {
            record.fields.forEach((f, idx) => {
              const suffix = idx < record.fields.length - 1 ? " +" : ";";
              b.$line(`GPUDatablock<${f.type}>::Size${suffix}`);
            })
          })
          b.$line(``)

          // build ctor
          b.$line(`inline GPUDatablock<${record.name}>(const ${record.name}& v)`)
          b.$indent(_b => {
            record.fields.forEach((f, idx) => {
              const prefix = idx === 0 ? ":" : ",";
              b.$line(`${prefix} _${f.short_name}(v.${f.short_name})`);
            });
          })
          b.$line(`{}`)
          b.$line(``)

          // build operator
          b.$line(`inline operator ${record.name}() const {`)
          b.$indent(_b => {
            b.$line(`${record.name} v;`)
            record.fields.forEach((f) => {
              b.$line(`v.${f.short_name} = _${f.short_name};`)
            });
            b.$line(`return v;`)
          })
          b.$line(`}`)
          b.$line(``)

          // build fields
          b.$indent_pop(_b => {
            b.$line(`private:`)
          })
          record.fields.forEach((f) => {
            b.$line(`GPUDatablock<${f.type}> _${f.short_name};`)
          })
        });
        b.$line(`};`)
      });
      b.$line(`//! END Data Blocks`)
    }
  }
  static filter_record(record: db.Record) {
    return record.ml_configs.gpu.enable;
  }
}

class GPUDataBlock extends gen.Generator {
  override inject_configs(): void {
    // record
    this.main_module_db.each_record((record) => {
      record.ml_configs.gpu = new RecordConfig();
    })
  }
  override gen(): void {
    // gen headers
    this.main_module_db.headers.forEach((header) => {
      _Gen.header_data_blocks(header);
    });
  }
}

export function load_generator(gm: gen.GenerateManager) {
  gm.add_generator("gpu_data_block", new GPUDataBlock());
}