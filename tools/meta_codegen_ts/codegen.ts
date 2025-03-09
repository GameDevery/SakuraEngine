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
const parser = ml.Compiler.load();

// compile attrs exprs
// proj_db.each_cpp_types((cpp_type, _header) => {
//   for (const attr of cpp_type.raw_attrs) {
//     const program = parser.parse(attr);
//     cpp_type.attrs.push(program);
//   }
// });

// combine attrs context

// run attrs expr
// proj_db.each_cpp_types((cpp_type, _header) => {
//   if (cpp_type instanceof cpp.Enum) {
//   } else if (cpp_type instanceof cpp.EnumValue) {
//   } else if (cpp_type instanceof cpp.Record) {
//   } else if (cpp_type instanceof cpp.Field) {
//   } else if (cpp_type instanceof cpp.Method) {
//   } else if (cpp_type instanceof cpp.Function) {
//   } else if (cpp_type instanceof cpp.Parameter) {
//   }
// });

// run codegen

class TestMetaLang {
  // value assign
  @ml.value('number')
  number_val: number = 0;
  @ml.value('boolean')
  boolean_val: boolean = false;
  @ml.value('string')
  string_val: string = "";

  // array assign
  @ml.array('number')
  number_arr_val: number[] = [];
  @ml.array('boolean')
  boolean_arr_val: boolean[] = [];
  @ml.array('string')
  string_arr_val: string[] = [];

  // preset
  @ml.preset('default')
  preset_default() {
    this.number_val = 0;
    this.boolean_val = false;
    this.string_val = "";

    this.number_arr_val = [];
    this.boolean_arr_val = [];
    this.string_arr_val = [];
  }

  // sub object
  sub: TestMetaLangSub = new TestMetaLangSub();
}
class TestMetaLangSub {
  @ml.value('number')
  value: number = 0;

  @ml.array('number')
  array_value: number[] = [];

  @ml.value_proxy('number')
  proxy_value(v: number) {
    this.value = v;
  }

  @ml.array_proxy('number')
  proxy_array(v: number[], is_append: boolean) {
    if (is_append) {
      this.array_value.push(...v);
    } else {
      this.array_value = v;
    }
  }
};
const compiler = ml.Compiler.load();
const program = compiler.compile(
  `
  test.number_val = 114;
  test.boolean_val = true;
  test.string_val = "114514";

  test = @default;

  test.number_arr_val = [1, 2, 3];
  test.boolean_arr_val = [true, false, true];
  test.string_arr_val = ["1", "2", "3"];

  test.sub = 114514;
  test.sub = [1, 1, 4, 5, 1, 4];
  `
);
const test_obj = {
  test: new TestMetaLang()
};
program.exec(test_obj);
console.log(test_obj);