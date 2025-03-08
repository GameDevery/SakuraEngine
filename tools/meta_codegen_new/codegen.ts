import * as config from "./framework/config.ts";
import * as db from "./framework/database.ts";
import * as ml from "./framework/meta_lang.ts";
import * as fs from "node:fs";
import * as process from "node:process";
import * as cpp from "./framework/cpp_types.ts";

const argv = process.argv.slice(2);

// parse args
if (argv.length != 1) {
  throw Error("only support 1 argument that config file path");
}
const config_file_path = argv[0];
if (!fs.existsSync(config_file_path)) {
  throw Error("config file is not exist");
}

// parse config
const codegen_config = new config.CodegenConfig(
  JSON.parse(fs.readFileSync(config_file_path, { encoding: "utf-8" })),
);

// load data base
const proj_db = new db.Project(codegen_config);

// load meta lang parser
const parser = ml.load_parser();

// compile attrs exprs
// proj_db.each_cpp_types((cpp_type, _header) => {
//   for (const attr of cpp_type.raw_attrs) {
//     const program = parser.parse(attr);
//     cpp_type.attrs.push(program);
//   }
// });

// combine attrs context

// run attrs expr
proj_db.each_cpp_types((cpp_type, _header) => {
  if (cpp_type instanceof cpp.Enum) {
  } else if (cpp_type instanceof cpp.EnumValue) {
  } else if (cpp_type instanceof cpp.Record) {
  } else if (cpp_type instanceof cpp.Field) {
  } else if (cpp_type instanceof cpp.Method) {
  } else if (cpp_type instanceof cpp.Function) {
  } else if (cpp_type instanceof cpp.Parameter) {
  }
});

// run codegen
