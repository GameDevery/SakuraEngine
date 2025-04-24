import * as process from "node:process";
import * as gen_vector from "./framework/gen_vector";
import path from "node:path";
import { CodeBuilder } from "./framework/util";

const argv = process.argv.slice(2);

// parse args
if (argv.length != 1) {
  throw Error("only support 1 argument that generate dir");
}
const gen_dir = argv[0]!;

// config
interface TypeOption {
  comp_name: string;
  comp_kind: gen_vector.EVectorCompKind;
}
export const allowed_types: Dict<TypeOption> = {
  "float": {
    comp_name: "float",
    comp_kind: gen_vector.EVectorCompKind.float,
  },
  "double": {
    comp_name: "double",
    comp_kind: gen_vector.EVectorCompKind.float,
  },
  "bool": {
    comp_name: "bool",
    comp_kind: gen_vector.EVectorCompKind.boolean,
  },
  "int": {
    comp_name: "int32_t",
    comp_kind: gen_vector.EVectorCompKind.integer,
  },
  "uint": {
    comp_name: "uint32_t",
    comp_kind: gen_vector.EVectorCompKind.integer,
  },
  "bigint": {
    comp_name: "int64_t",
    comp_kind: gen_vector.EVectorCompKind.integer,
  },
  "ubigint": {
    comp_name: "uint64_t",
    comp_kind: gen_vector.EVectorCompKind.integer,
  },
}

// fwd builder
let fwd_builder = new CodeBuilder();
fwd_builder.$util_header();

// generate vector types
for (const base_name in allowed_types) {
  const type_opt = allowed_types[base_name]!;

  // header
  let vector_builder = new CodeBuilder()
  vector_builder.$util_header();

  // include
  vector_builder.$line(`#include <cstdint>`);
  vector_builder.$line(`#include <cmath>`);
  vector_builder.$line(`#include "../gen_math_fwd.hpp"`);
  vector_builder.$line(`#include <SkrBase/misc/debug.h>`);
  vector_builder.$line(`#include <SkrBase/misc/hash.hpp>`);
  vector_builder.$line(``);

  // gen vector code
  gen_vector.gen({
    fwd_builder: fwd_builder,
    builder: vector_builder,
    base_name: base_name,
    component_name: type_opt.comp_name,
    component_kind: type_opt.comp_kind,
  })

  // write to file
  const out_path = path.join(gen_dir, "vec", `${base_name}_vec.hpp`);
  vector_builder.write_file(out_path);
}


// write forward declaration
const fwd_out_path = path.join(gen_dir, "gen_math_fwd.hpp");
fwd_builder.write_file(fwd_out_path);