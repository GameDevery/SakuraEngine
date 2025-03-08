export class ModuleConfig {
  module_name: string;
  meta_dir: string;
  api: string;

  // deno-lint-ignore no-explicit-any
  constructor(json_obj: any) {
    this.module_name = json_obj.module_name;
    this.meta_dir = json_obj.meta_dir;
    this.api = json_obj.api;
  }
}

export class GeneratorConfig {
  entry_file: string;
  import_dir: string[];

  // deno-lint-ignore no-explicit-any
  constructor(json_obj: any) {
    this.entry_file = json_obj.entry_file;
    this.import_dir = json_obj.import === undefined ? [] : json_obj.import;
  }
}

export class CodegenConfig {
  output_dir: string;
  main_module: ModuleConfig;
  include_modules: ModuleConfig[] = [];
  generators: GeneratorConfig[] = [];

  // deno-lint-ignore no-explicit-any
  constructor(json_obj: any) {
    this.output_dir = json_obj.output_dir;

    // load main module
    this.main_module = new ModuleConfig(json_obj.main_module);

    // load include modules
    if (json_obj.include_modules !== undefined) {
      for (const module of json_obj.include_modules) {
        this.include_modules.push(new ModuleConfig(module));
      }
    }

    // load generators
    if (json_obj.generators !== undefined) {
      for (const generator of json_obj.generators) {
        this.generators.push(new GeneratorConfig(generator));
      }
    }
  }
}
