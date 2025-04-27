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
        b.$line(``)

        // variable based
        b.$line(`// vector based`)
        b.$line(`struct {`)
        b.$indent(_b => {
          for (let col_idx = 0; col_idx < dim; ++col_idx) {
            const row_members = dims_all
              .slice(0, dim)
              .map(d => `m${d - 1}${col_idx}`)
              .join(`, `);
            b.$line(`${comp_name} ${row_members};`)
          }
        })
        b.$line(`};`)
      })
      b.$line(`};`)

      // ctor & dtor
      {
        b.$line(`// ctor & dtor`)
        const default_ctor_exprs = `: ` + _axis_lut
          .slice(0, dim)
          .map(axis => `axis_${axis}(0)`)
          .join(`, `);
        // const default_ctor_exprs = `/*do noting for performance purpose, use factory init will be better*/`
        b.$line(`inline ${mat_name}() ${default_ctor_exprs} {}`)
        b.$line(`inline ${mat_name}(`)
        b.$indent(_b => {
          for (let row_idx = 0; row_idx < dim; ++row_idx) {
            const row_members = dims_all
              .slice(0, dim)
              .map(d => `${comp_name} m${row_idx}${d - 1}`)
              .join(`, `);
            if (row_idx === dim - 1) {
              b.$line(`${row_members}`)
            } else {
              b.$line(`${row_members},`)
            }
          }
        })
        b.$line(`):`)
        b.$indent(_b => {
          for (let col_idx = 0; col_idx < dim; ++col_idx) {
            const row_members = dims_all
              .slice(0, dim)
              .map(d => `m${d - 1}${col_idx}(m${d - 1}${col_idx})`)
              .join(`, `);
            if (col_idx === dim - 1) {
              b.$line(`${row_members}`)
            }
            else {
              b.$line(`${row_members},`)
            }
          }
        })
        b.$line(`{}`)
        {
          const axis_params = _axis_lut
            .slice(0, dim)
            .map(axis => `${vec_name} axis_${axis}`)
            .join(`, `);
          const axis_init_list = _axis_lut
            .slice(0, dim)
            .map(axis => `axis_${axis}(axis_${axis})`)
            .join(`, `);

          b.$line(`inline ${mat_name}(${axis_params}) noexcept : ${axis_init_list} {}`)
        }
        b.$line(`inline ~${mat_name}() = default;`)
        b.$line(``)
      }

      // factory
      {
        b.$line(`// factory`)
        b.$line(`inline static ${mat_name} eye(${comp_name} v) {`)
        b.$indent(_b => {
          b.$line(`return ${mat_name}{`)
          b.$indent(_b => {
            for (let row_idx = 0; row_idx < dim; ++row_idx) {
              const row_members = dims_all
                .slice(0, dim)
                .map(d => d === row_idx + 1 ? `v` : `0`)
                .join(`, `);
              if (row_idx === dim - 1) {
                b.$line(`${row_members}`)
              } else {
                b.$line(`${row_members},`)
              }
            }
          })
          b.$line(`};`)
        })
        b.$line(`}`)
        b.$line(`inline static ${mat_name} fill(${comp_name} v) {`)
        b.$indent(_b => {
          b.$line(`return ${mat_name}{`)
          b.$indent(_b => {
            for (let row_idx = 0; row_idx < dim; ++row_idx) {
              const row_members = dims_all
                .slice(0, dim)
                .map(d => `v`)
                .join(`, `);
              if (row_idx === dim - 1) {
                b.$line(`${row_members}`)
              } else {
                b.$line(`${row_members},`)
              }
            }
          })
          b.$line(`};`)
        })
        b.$line(`}`)
        b.$line(`inline static ${mat_name} identity() { return eye(1); }`)
        b.$line(`inline static ${mat_name} zero() { return fill(0); }`)
        b.$line(`inline static ${mat_name} one() { return fill(1); }`)
        b.$line(``)
      }

      // copy & move & assign & move assign
      {
        b.$line(`// copy & move & assign & move assign`)
        const copy_exprs = _axis_lut
          .slice(0, dim)
          .map(axis => `axis_${axis}(rhs.axis_${axis})`)
          .join(`, `);
        b.$line(`inline ${mat_name}(const ${mat_name}& rhs) noexcept : ${copy_exprs} {}`)
        const move_exprs = _axis_lut
          .slice(0, dim)
          .map(axis => `axis_${axis}(std::move(rhs.axis_${axis}))`)
          .join(`, `);
        b.$line(`inline ${mat_name}(${mat_name}&& rhs) noexcept : ${move_exprs} {}`)
        const assign_exprs = _axis_lut
          .slice(0, dim)
          .map(axis => `this->axis_${axis} = rhs.axis_${axis};`)
          .join(` `);
        b.$line(`inline ${mat_name}& operator=(const ${mat_name}& rhs) noexcept { ${assign_exprs} return *this; }`)
        const move_assign_exprs = _axis_lut
          .slice(0, dim)
          .map(axis => `this->axis_${axis} = std::move(rhs.axis_${axis});`)
          .join(` `);
        b.$line(`inline ${mat_name}& operator=(${mat_name}&& rhs) noexcept { ${move_assign_exprs} return *this; }`)
        b.$line(``)
      }

      // mul operator
      b.$line(`// mul operator`)
      b.$line(`friend ${mat_name} operator*(const ${mat_name}& lhs, const ${mat_name}& rhs);`)
      b.$line(`${mat_name}& operator*=(const ${mat_name}& rhs);`)
      b.$line(`friend ${vec_name} operator*(const ${vec_name}& lhs, const ${mat_name}& rhs);`)

    })
    b.$line(`};`)
  }
}


export function gen(fwd_builder: CodeBuilder, gen_dir: string) {
  const inc_builder = new CodeBuilder();
  inc_builder.$util_header();

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
    builder.$line(`inline namespace math {`);

    _gen_class_body({
      fwd_builder,
      builder,
      base_name,
      ...type_opt
    })
    builder.$line(`}`)
    builder.$line(`}`)

    // write to file
    const file_name = path.join(gen_dir, `${base_name}_matrix.hpp`);
    builder.write_file(file_name)
    inc_builder.$line(`#include "./${path.basename(file_name)}"`);
  }

  // write include file
  const file_name = path.join(gen_dir, `gen_matrix.hpp`);
  inc_builder.write_file(file_name);
}