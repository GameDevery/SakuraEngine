import * as cpp from "./cpp_types.ts";
import * as config from "./config.ts";
import * as path from "node:path";
import * as fs from "node:fs";
import "./utils.ts"
import { CodeBuilder } from "./utils.ts";

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

  gen_code: CodeBuilder = new CodeBuilder();

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
    this.file_id = `FID_${parent.config.module_name}_${this.meta_path_relative.replaceAll(reg_non_alpha_ch, "_")
      }`;
    const reg_meta_path = /(.*?)\.(.*?)\.meta/g;
    this.output_header_path = this.meta_path_relative.replace(
      reg_meta_path,
      "$1.generated.$2.h",
    );

    // load json
    const json_obj = JSON.parse(
      fs.readFileSync(this.meta_path, { encoding: "utf-8" }),
    );
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

  // find
  find_record(name: string) {
    for (const record of this.records) {
      if (record.name == name) {
        return record;
      }
    }
    return null;
  }
  find_enum(name: string) {
    for (const enum_obj of this.enums) {
      if (enum_obj.name == name) {
        return enum_obj;
      }
    }
    return null;
  }
  find_function(name: string) {
    for (const function_obj of this.functions) {
      if (function_obj.name == name) {
        return function_obj;
      }
    }
    return null;
  }

  // each functions
  each_record(
    func: (record: cpp.Record, header: Header) => void,
  ) {
    for (const record of this.records) {
      func(record, this);
    }
  }
  each_enum(
    func: (enum_obj: cpp.Enum, header: Header) => void,
  ) {
    for (const enum_obj of this.enums) {
      func(enum_obj, this);
    }
  }
  each_function(
    func: (function_obj: cpp.Function, header: Header) => void,
  ) {
    for (const function_obj of this.functions) {
      func(function_obj, this);
    }
  }
  each_cpp_types(
    func: (cpp_type: cpp.AnyType, header: Header) => void,
  ) {
    for (const record of this.records) {
      func(record, this);

      for (const method of record.methods) {
        func(method, this);
        for (const param of method.parameters) {
          func(param, this);
        }
      }

      for (const field of record.fields) {
        func(field, this);
      }
    }
    for (const enum_obj of this.enums) {
      func(enum_obj, this);
    }
    for (const function_obj of this.functions) {
      func(function_obj, this);
      for (const param of function_obj.parameters) {
        func(param, this);
      }
    }
  }
}

export class Module {
  parent: Project;
  config: config.ModuleConfig;
  headers: Header[] = [];
  gen_code: CodeBuilder = new CodeBuilder();

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

  // find
  find_record(name: string) {
    for (const header of this.headers) {
      const record = header.find_record(name);
      if (record) {
        return record;
      }
    }
    return null;
  }
  find_enum(name: string) {
    for (const header of this.headers) {
      const enum_obj = header.find_enum(name);
      if (enum_obj) {
        return enum_obj;
      }
    }
    return null;
  }
  find_function(name: string) {
    for (const header of this.headers) {
      const function_obj = header.find_function(name);
      if (function_obj) {
        return function_obj;
      }
    }
    return null;
  }

  // each functions
  each_record(
    func: (record: cpp.Record, header: Header) => void,
  ) {
    for (const header of this.headers) {
      header.each_record(func);
    }
  }
  each_enum(
    func: (enum_obj: cpp.Enum, header: Header) => void,
  ) {
    for (const header of this.headers) {
      header.each_enum(func);
    }
  }
  each_function(
    func: (function_obj: cpp.Function, header: Header) => void,
  ) {
    for (const header of this.headers) {
      header.each_function(func);
    }
  }
  each_cpp_types(
    func: (cpp_type: cpp.AnyType, header: Header) => void,
  ) {
    for (const header of this.headers) {
      header.each_cpp_types(func);
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

  // find
  find_record(name: string) {
    let record = this.main_module.find_record(name);
    if (record) {
      return record;
    }
    for (const include_module of this.include_modules) {
      record = include_module.find_record(name);
      if (record) {
        return record;
      }
    }
    return null;
  }
  find_enum(name: string) {
    let enum_obj = this.main_module.find_enum(name);
    if (enum_obj) {
      return enum_obj;
    }
    for (const include_module of this.include_modules) {
      enum_obj = include_module.find_enum(name);
      if (enum_obj) {
        return enum_obj;
      }
    }
    return null;
  }
  find_function(name: string) {
    let function_obj = this.main_module.find_function(name);
    if (function_obj) {
      return function_obj;
    }
    for (const include_module of this.include_modules) {
      function_obj = include_module.find_function(name);
      if (function_obj) {
        return function_obj;
      }
    }
    return null;
  }

  // each functions
  each_record(
    func: (record: cpp.Record, header: Header) => void,
  ) {
    this.main_module.each_record(func);
    for (const include_module of this.include_modules) {
      include_module.each_record(func);
    }
  }
  each_enum(
    func: (enum_obj: cpp.Enum, header: Header) => void,
  ) {
    this.main_module.each_enum(func);
    for (const include_module of this.include_modules) {
      include_module.each_enum(func);
    }
  }
  each_function(
    func: (function_obj: cpp.Function, header: Header) => void,
  ) {
    this.main_module.each_function(func);
    for (const include_module of this.include_modules) {
      include_module.each_function(func);
    }
  }
  each_cpp_types(
    func: (cpp_type: cpp.AnyType, header: Header) => void,
  ) {
    this.main_module.each_cpp_types(func);
    for (const include_module of this.include_modules) {
      include_module.each_cpp_types(func);
    }
  }

  // tool functions
  is_derived(record: cpp.Record, base: string) {
    for (const base_name of record.bases) {
      if (base_name == base) {
        return true;
      }
      const base_record = this.find_record(base_name);
      if (base_record != null && this.is_derived(base_record, base)) {
        return true;
      }
    }
  }
  is_derived_or_same(record: cpp.Record, base: string) {
    return record.name == base || this.is_derived(record, base);
  }
}
