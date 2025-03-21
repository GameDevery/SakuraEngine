import * as db from "./framework/database.ts";
import * as ml from "./framework/meta_lang.ts";
import * as gen from "./framework/generator"
import * as fs from "node:fs";
import * as process from "node:process";

const argv = process.argv.slice(2);

// parse args
if (argv.length != 1) {
  throw Error("only support 1 argument that config file path");
}
const config_file_path = argv[0];

const generator = new gen.GenerateManager();

// load database
generator.load_database(config_file_path);

// load generators
await generator.load_generators();

// inject configs
generator.inject_configs();

// run meta-lang
generator.run_meta()

// run codegen
generator.codegen()

// output
generator.output()


