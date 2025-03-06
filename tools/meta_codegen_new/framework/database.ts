import * as cpp from "./cpp_types.ts";

export class Header {
  parent: Module;

  // header file path
  meta_path: string = "";
  meta_path_relative: string = "";
  header_path: string = "";
  header_path_relative: string = "";
  file_id: string = "";

  // data
  records: cpp.Record[] = [];
  enums: cpp.Enum[] = [];
  functions: cpp.Function[] = [];

  constructor(parent: Module) {
    this.parent = parent;
  }
}

export class Module {
  parent: Project;

  // module info
  module_name: string = "";
  meta_dir: string = "";
  api: string = "";

  // headers
  headers: Header[] = [];

  constructor(parent: Project) {
    this.parent = parent;
  }
}

export class Project {
  main_module: Module = new Module(this);
  include_modules: Module[] = [];

  constructor() {}
}
