import {
  CodeBuilder, type_convert_options,
  type_options, all_component_kinds,
  dims_all, dims_no_scalar,
  matrix_dims
} from "./util"
import type { TypeOption, ComponentKind } from "./util";
import path from "node:path";

const _axis_lut = ["x", "y", "z", "w"]

interface GenMatrixOption extends TypeOption {
  fwd_builder: CodeBuilder;
  builder: CodeBuilder;
  base_name: string    // [basename][2/3/4]
};

function _gen_class_body(opt: GenMatrixOption) {
  const fwd_b = opt.fwd_builder;
  const b = opt.builder;
  const base_name = opt.base_name;
  const comp_name = opt.component_name;
  const comp_kind = opt.component_kind;

  // generate forward declaration
  fwd_b.$line(`// ${base_name} matrix, component: ${comp_name}`)
  for (const dim of matrix_dims) {
    fwd_b.$line(`struct ${base_name}${dim}x${dim};`)
  }
  fwd_b.$line(``);

  // generate class body
  for (const dim of matrix_dims) {
    const mat_name = `${base_name}${dim}x${dim}`;
    const vec_name = `${base_name}${dim}`;

    b.$line(`struct ${mat_name} {`)
    b.$indent(_b => {
      // fill component types
      b.$line(`union {`)
      b.$indent(_b => {
        // axis based
        b.$line(`// base axis`)
        b.$line(`struct {`)
        b.$indent(_b => {
          for (let axis_idx = 0; axis_idx < dim; ++axis_idx) {
            b.$line(`${vec_name} axis_${_axis_lut[axis_idx]};`)
          }
        })
        b.$line(`};`)
        b.$line(``)

        // column based
        b.$line(`// base columns`)
        b.$line(`${vec_name} columns[${dim}];`)

        // 

      })
      b.$line(`};`)


    })
    b.$line(`};`)
  }
}


export function gen(fwd_builder: CodeBuilder, gen_dir: string) {
  for (const base_name in type_options) {
    const type_opt = type_options[base_name]!;

    // filter component kinds
    if (type_opt.component_kind !== "floating") continue;

    // header
    const builder = new CodeBuilder();
    builder.$util_header();

    // include
    builder.$line(`#include <cstdint>`);
    builder.$line(`#include <cmath>`);
    builder.$line(`#include "../gen_math_fwd.hpp"`);
    builder.$line(`#include "../vec/${base_name}_vec.hpp"`);
    builder.$line(`#include <SkrBase/misc/debug.h>`);
    builder.$line(`#include <SkrBase/misc/hash.hpp>`);
    builder.$line(``);

    // gen code
    builder.$line(`namespace skr {`);
    _gen_class_body({
      fwd_builder,
      builder,
      base_name,
      ...type_opt
    })
    builder.$line(`}`)

    // write to file
    const file_name = path.join(gen_dir, `${base_name}_matrix.h`);
    builder.write_file(file_name)
  }
}