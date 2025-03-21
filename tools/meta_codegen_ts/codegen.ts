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

// TODO. use ErrorTracker to track meta-lang error, instead of using throw
// TODO. use low level api to print log
// class LogHelper {
//   static red(str: string) {
//     return `\x1b[31m${str}\x1b[0m`;
//   }
//   static green(str: string) {
//     return `\x1b[32m${str}\x1b[0m`;
//   }
//   static yellow(str: string) {
//     return `\x1b[33m${str}\x1b[0m`;
//   }
//   static blue(str: string) {
//     return `\x1b[34m${str}\x1b[0m`;
//   }
//   static magenta(str: string) {
//     return `\x1b[35m${str}\x1b[0m`;
//   }
//   static cyan(str: string) {
//     return `\x1b[36m${str}\x1b[0m`;
//   }
//   static white(str: string) {
//     return `\x1b[37m${str}\x1b[0m`;
//   }
//   static gray(str: string) {
//     return `\x1b[30m${str}\x1b[0m`;
//   }
// }
// await Bun.write(Bun.stderr, LogHelper.red("red\n"))
// await Bun.write(Bun.stderr, LogHelper.green("green\n"))
// await Bun.write(Bun.stderr, LogHelper.yellow("yellow\n"))
// await Bun.write(Bun.stderr, LogHelper.blue("blue\n"))
// await Bun.write(Bun.stderr, LogHelper.magenta("magenta\n"))
// await Bun.write(Bun.stderr, LogHelper.cyan("cyan\n"))
// await Bun.write(Bun.stderr, LogHelper.white("white\n"))
// await Bun.write(Bun.stderr, LogHelper.gray("gray\n"))

// TODO. use process.exit control output manually
// process.exit()