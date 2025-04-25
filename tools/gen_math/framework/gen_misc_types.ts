import {
  CodeBuilder, type_convert_options,
  type_options, all_component_kinds,
  dims_all, dims_no_scalar
} from "./util"
import type { TypeOption, ComponentKind } from "./util";
import path from "node:path";

const _suffix_lut: Dict<string> = {
  "float": "F",
  "double": "D",
}
const _quat_comp_lut = ["x", "y", "z", "w"];

interface GenMiscOption {
  fwd_builder: CodeBuilder;
  builder: CodeBuilder;
};

function _gen_quat(opt: GenMiscOption) {
  const fwd_b = opt.fwd_builder;
  const b = opt.builder;

  // generate forward declaration
  fwd_b.$line(`// quaternion`)
  for (const base_name in type_options) {
    const type_opt = type_options[base_name]!;

    // filter component kinds
    if (type_opt.component_kind !== "floating") continue;

    // get suffix
    const suffix = _suffix_lut[type_opt.component_name];
    if (suffix === undefined) {
      throw new Error(`unknown component name ${type_opt.component_name} for quat`);
    }

    fwd_b.$line(`struct Quat${suffix};`);
  }
  fwd_b.$line(``);

  // generate quat body
  for (const base_name in type_options) {
    const type_opt = type_options[base_name]!;

    // filter component kinds
    if (type_opt.component_kind !== "floating") continue;

    // get suffix
    const suffix = _suffix_lut[type_opt.component_name];
    if (suffix === undefined) {
      throw new Error(`unknown component name ${type_opt.component_name} for quat`);
    }

    // get data
    const comp_name = type_opt.component_name;
    const quat_name = `Quat${suffix}`;

    b.$line(`struct ${quat_name} {`)
    b.$indent(_b => {
      // member
      b.$line(`${comp_name} ${_quat_comp_lut.join(`, `)};`)
      b.$line(``)

      // ctor & dtor
      b.$line(`// ctor & dtor`)
      b.$line(`inline ${quat_name}() : x(0), y(0), z(0), w(1) {}`)
      {
        const ctor_params = _quat_comp_lut
          .map(c => `${comp_name} ${c}`)
          .join(`, `);
        const init_list = _quat_comp_lut
          .map(c => `${c}(${c})`)
          .join(`, `);
        b.$line(`inline ${quat_name}(${ctor_params}) : ${init_list} {}`)
      }
      b.$line(`inline ~${quat_name}() = default;`)
      b.$line(``)

      // factory
      b.$line(`// factory`)
      b.$line(`inline static ${quat_name} Identity() { return { 0, 0, 0, 1 }; }`)
      b.$line(`inline static ${quat_name} Fill(${comp_name} v) { return { v, v, v, v }; }`)
      b.$line(``)

      // copy & move & assign & move assign
      b.$line(`// copy & move & assign & move assign`)
      b.$line(`inline ${quat_name}(${quat_name} const&) = default;`)
      b.$line(`inline ${quat_name}(${quat_name}&&) = default;`)
      b.$line(`inline ${quat_name}& operator=(${quat_name} const&) = default;`)
      b.$line(`inline ${quat_name}& operator=(${quat_name}&&) = default;`)

    })
    b.$line(`};`)
  }
}

function _gen_rotator(opt: GenMiscOption) {
  const fwd_b = opt.fwd_builder;
  const b = opt.builder;

  // generate forward declaration
  fwd_b.$line(`// rotator`)
  for (const base_name in type_options) {
    const type_opt = type_options[base_name]!;

    // filter component kinds
    if (type_opt.component_kind !== "floating") continue;

    // get suffix
    const suffix = _suffix_lut[type_opt.component_name];
    if (suffix === undefined) {
      throw new Error(`unknown component name ${type_opt.component_name} for quat`);
    }

    fwd_b.$line(`struct Rotator${suffix};`);
  }
  fwd_b.$line(``);
}

function _gen_transform(opt: GenMiscOption) {
  const fwd_b = opt.fwd_builder;
  const b = opt.builder;

  // generate forward declaration
  fwd_b.$line(`// transform`)
  for (const base_name in type_options) {
    const type_opt = type_options[base_name]!;

    // filter component kinds
    if (type_opt.component_kind !== "floating") continue;

    // get suffix
    const suffix = _suffix_lut[type_opt.component_name];
    if (suffix === undefined) {
      throw new Error(`unknown component name ${type_opt.component_name} for quat`);
    }

    fwd_b.$line(`struct Transform${suffix};`);
  }
  fwd_b.$line(``);
}

function _gen_camera(opt: GenMiscOption) {
  const fwd_b = opt.fwd_builder;
  const b = opt.builder;

  // generate forward declaration
  fwd_b.$line(`// transform`)
  for (const base_name in type_options) {
    const type_opt = type_options[base_name]!;

    // filter component kinds
    if (type_opt.component_kind !== "floating") continue;

    // get suffix
    const suffix = _suffix_lut[type_opt.component_name];
    if (suffix === undefined) {
      throw new Error(`unknown component name ${type_opt.component_name} for quat`);
    }

    fwd_b.$line(`struct Camera${suffix};`);
  }
  fwd_b.$line(``);
}

export function gen(fwd_builder: CodeBuilder, gen_dir: string) {
  // generate quat
  {
    // header
    const builder = new CodeBuilder();
    builder.$util_header();

    // include
    builder.$line(`#include <cstdint>`);
    builder.$line(`#include <cmath>`);
    builder.$line(`#include "../gen_math_fwd.hpp"`);
    builder.$line(`#include <SkrBase/misc/debug.h>`);
    builder.$line(`#include <SkrBase/misc/hash.hpp>`);
    builder.$line(``);

    // gen code
    builder.$line(`namespace skr {`);
    _gen_quat({
      fwd_builder,
      builder,
    })
    builder.$line(`}`)

    // write to file
    const file_name = path.join(gen_dir, `quat.hpp`);
    builder.write_file(file_name);
  }

  // generate rotator
  {
    const builder = new CodeBuilder();
    builder.$util_header();

    // include
    builder.$line(`#include <cstdint>`);
    builder.$line(`#include <cmath>`);
    builder.$line(`#include "../gen_math_fwd.hpp"`);
    builder.$line(`#include <SkrBase/misc/debug.h>`);

    // gen code
    builder.$line(`namespace skr {`);
    _gen_rotator({
      fwd_builder,
      builder,
    })
    builder.$line(`}`)

    // write to file
    const file_name = path.join(gen_dir, `rotator.hpp`);
    builder.write_file(file_name);
  }

  // generate transform
  {
    const builder = new CodeBuilder();
    builder.$util_header();

    // include
    builder.$line(`#include <cstdint>`);
    builder.$line(`#include <cmath>`);
    builder.$line(`#include "../gen_math_fwd.hpp"`);
    builder.$line(`#include <SkrBase/misc/debug.h>`);

    // gen code
    builder.$line(`namespace skr {`);
    _gen_transform({
      fwd_builder,
      builder,
    })
    builder.$line(`}`)

    // write to file
    const file_name = path.join(gen_dir, `transform.hpp`);
    builder.write_file(file_name);
  }

  // generate camera
  {
    const builder = new CodeBuilder();
    builder.$util_header();

    // include
    builder.$line(`#include <cstdint>`);
    builder.$line(`#include <cmath>`);
    builder.$line(`#include "../gen_math_fwd.hpp"`);
    builder.$line(`#include <SkrBase/misc/debug.h>`);

    // gen code
    builder.$line(`namespace skr {`);
    _gen_camera({
      fwd_builder,
      builder,
    })
    builder.$line(`}`)

    // write to file
    const file_name = path.join(gen_dir, `camera.hpp`);
    builder.write_file(file_name);
  }
}