import * as process from "node:process";
import * as gen_vector from "./framework/gen_vector";
import * as gen_math_func from "./framework/gen_math_func";
import * as gen_matrix from "./framework/gen_matrix";
import path from "node:path";
import { CodeBuilder } from "./framework/util";

const argv = process.argv.slice(2);

// parse args
if (argv.length != 1) {
  throw Error("only support 1 argument that generate dir");
}
const gen_dir = argv[0]!;

// fwd builder
let fwd_builder = new CodeBuilder();
fwd_builder.$util_header();
fwd_builder.$line("");
fwd_builder.$line("namespace skr {");

// generate vector types
gen_vector.gen(
  fwd_builder,
  path.join(gen_dir, "vec")
)

// generate matrix types
gen_matrix.gen(
  fwd_builder,
  path.join(gen_dir, "mat")
)

// generate math functions
gen_math_func.gen(
  fwd_builder,
  path.join(gen_dir, "math")
)

// write forward declaration
fwd_builder.$line("}");
const fwd_out_path = path.join(gen_dir, "gen_math_fwd.hpp");
fwd_builder.write_file(fwd_out_path);