import * as config from "./framework/config.ts";

// parse args
if (Deno.args.length != 1) {
  console.error("only support 1 argument that config file path");
}
const config_file_path = Deno.args[0];
if (Deno.lstatSync(config_file_path).isFile == false) {
  console.error("config file is not exist");
}

// parse config
const codegen_config = new config.CodegenConfig(
  JSON.parse(Deno.readTextFileSync(config_file_path)),
);
// console.log(codegen_config);

// load data base
