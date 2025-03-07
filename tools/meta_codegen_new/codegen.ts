import * as config from "./framework/config.ts";
import * as db from "./framework/database.ts";
import * as ml from "./framework/meta_lang.ts";
import * as fs from "node:fs";

const argv = process.argv.slice(2);

// parse args
if (argv.length != 1) {
  console.error("only support 1 argument that config file path");
}
const config_file_path = argv[0];
if (!fs.existsSync(config_file_path)) {
  console.error("config file is not exist");
}

// parse config
const codegen_config = new config.CodegenConfig(
  JSON.parse(fs.readFileSync(config_file_path, { encoding: "utf-8" })),
);

// load data base
const proj_db = new db.Project(codegen_config);

// load meta lang parser
let parser = ml.load_parser();
