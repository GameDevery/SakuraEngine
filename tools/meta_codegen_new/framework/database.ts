import * as cpp from "./cpp_types.ts";
import * as config from "./config.ts";
import * as path from "node:path";
import * as fs from "node:fs";

export class Header {
  parent: Module;

  // meta source info
  meta_path: string = "";
  meta_path_relative: string = "";

  // codegen info
  file_id: string = ""; // 用于 codegen 时候定义 FILE_ID 宏
  output_header_path: string = ""; // 用于 codegen 时输出的 header
  header_path: string = ""; // 用于 codegen 时 include 对应的 header

  // data
  records: cpp.Record[] = [];
  enums: cpp.Enum[] = [];
  functions: cpp.Function[] = [];

  constructor(parent: Module, meta_file_path: string) {
    this.parent = parent;

    // solve meta path
    this.meta_path = path.normalize(meta_file_path).replaceAll(
      path.sep,
      "/",
    );
    this.meta_path_relative = path.relative(
      parent.config.meta_dir,
      this.meta_path,
    );

    // solve output header path and file id
    const reg_non_alpha_ch = /\W+/g;
    this.file_id = `FID_${parent.config.module_name}_${
      this.meta_path_relative.replaceAll(reg_non_alpha_ch, "_")
    }`;
    const reg_meta_path = /(.*?)\.(.*?)\.meta/g;
    this.output_header_path = this.meta_path_relative.replace(
      reg_meta_path,
      "$1.generated.$2.h",
    );

    // load json
    const json_obj = JSON.parse(Deno.readTextFileSync(this.meta_path));
    for (const record_json of json_obj.records) {
      this.records.push(new cpp.Record(record_json));
    }
    for (const enum_json of json_obj.enums) {
      this.enums.push(new cpp.Enum(enum_json));
    }
    for (const function_json of json_obj.functions) {
      this.functions.push(new cpp.Function(function_json));
    }

    // solve include path
    const solve_include_path = (file_name: string) => {
      file_name = file_name.replaceAll(path.sep, "/");
      if (this.header_path && this.header_path !== file_name) {
        throw new Error(
          `header path is not same, ${this.header_path} != ${file_name}`,
        );
      }
      this.header_path = file_name;
    };
    for (const record of this.records) {
      solve_include_path(record.file_name);
    }
    for (const enum_obj of this.enums) {
      solve_include_path(enum_obj.file_name);
    }
    for (const function_obj of this.functions) {
      solve_include_path(function_obj.file_name);
    }
  }
}

export class Module {
  parent: Project;

  config: config.ModuleConfig;

  // headers
  headers: Header[] = [];

  constructor(parent: Project, config: config.ModuleConfig) {
    this.parent = parent;
    this.config = config;

    // glob meta files
    const meta_files = fs.globSync(
      path.join(config.meta_dir, "**", "*.h.meta"),
    );

    // load headers
    for (const meta_file of meta_files) {
      this.headers.push(new Header(this, meta_file));
    }
  }
}

export class Project {
  main_module: Module;
  include_modules: Module[] = [];
  config: config.CodegenConfig;

  constructor(
    config: config.CodegenConfig,
    load_include_module: boolean = false,
  ) {
    this.config = config;

    // load main module
    this.main_module = new Module(this, config.main_module);

    // load include modules
    if (load_include_module) {
      for (const include_module of config.include_modules) {
        this.include_modules.push(new Module(this, include_module));
      }
    }
  }
}
