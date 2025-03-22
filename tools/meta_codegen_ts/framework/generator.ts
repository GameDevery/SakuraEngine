import * as db from "./database.ts";
import * as fs from "node:fs";
import * as ml from "./meta_lang.ts";
import path from "node:path";
import { LogColor, MetaLangError, type CodeBuilder } from "./utils.ts";

// TODO. 重组生成步骤
//   1. load database
//   2. inject configs
//   3. run meta-lang
//   4. generate code

export class Generator {
  owner!: GenerateManager;
  get main_module_db(): db.Module {
    return this.owner.project_db.main_module;
  }
  get project_db(): db.Project {
    return this.owner.project_db;
  }

  // inject configs into object
  inject_configs(): void { }


  // generate functions
  gen_body() { }
  pre_gen() { }
  gen() { }
  post_gen() { }
}

export class GenerateManager {
  project_db!: db.Project;
  #generators: Dict<Generator> = {};

  // step 1. load database
  load_database(config_path: string) {
    // check config file
    if (!fs.existsSync(config_path)) {
      throw Error(`config file is not exist, path: ${config_path}`);
    }

    // parse config
    const codegen_config = new db.CodegenConfig(
      JSON.parse(fs.readFileSync(config_path, { encoding: "utf-8" })),
    );

    // load data base
    this.project_db = new db.Project(codegen_config, true);
  }

  // step 2. load generators
  async load_generators() {
    const import_tasks: Promise<any>[] = [];

    // dynamic import generators
    for (const gen of this.project_db.config.generators) {
      import_tasks.push(import(gen.entry_file));
    }

    // wait import done
    const modules = await Promise.all(import_tasks);

    // load generators
    for (const module of modules) {
      module.load_generator(this);
    }
  }

  // step 3. inject configs
  inject_configs() {
    for (const gen in this.#generators) {
      this.#generators[gen]?.inject_configs();
    }
  }

  // step 4. run meta-lang
  async run_meta() {
    const compiler = ml.Compiler.load();

    const meta_errors: MetaLangError[] = []

    // compile meta
    this.project_db.main_module.each_cpp_types((cpp_type) => {
      for (const attr of cpp_type.raw_attrs) {
        try {
          cpp_type.ml_programs.push(compiler.compile(attr));
        } catch (e) {
          if (e instanceof ml.CompileError) {
            meta_errors.push(new MetaLangError(
              `${cpp_type.file_name}:${cpp_type.line}`,
              e.message,
              attr,
              e.start,
              e.end,
            ));
          }
          else {
            throw e;
          }
        }
      }
    });

    // log meta lang and exit
    if (meta_errors.length > 0) {
      for (const error of meta_errors) {
        await error.write_log();
      }
      process.exit(1);
    }

    // run meta
    this.project_db.main_module.each_cpp_types((cpp_type) => {
      for (const program of cpp_type.ml_programs) {
        try {
          program.exec(cpp_type.ml_configs);
        } catch (e) {
          const cpp_source_loc = `${cpp_type.file_name}:${cpp_type.line}`;
          if (e instanceof ml.AccessFailedError) {
            meta_errors.push(new MetaLangError(
              cpp_source_loc,
              `failed to access '${e.key}'`,
              program.source,
              e.location.start,
              e.location.end
            ))
          } else if (e instanceof ml.TypeMismatchError) {
            meta_errors.push(new MetaLangError(
              cpp_source_loc,
              `type mismatch, expect ${e.expected}, but got ${e.actual}`,
              program.source,
              e.location.start,
              e.location.end
            ))
          } else if (e instanceof ml.PresetNotFoundError) {
            meta_errors.push(new MetaLangError(
              cpp_source_loc,
              `preset not found, name: '${e.preset}'`,
              program.source,
              e.location.start,
              e.location.end
            ))
          } else {
            throw e;
          }
        }
      }
    });

    // log meta lang and exit
    if (meta_errors.length > 0) {
      for (const error of meta_errors) {
        await error.write_log();
      }
      process.exit(1);
    }
  }

  // step 5. generate code
  codegen() {
    // generate body
    for (const key in this.#generators) {
      this.#generators[key]!.gen_body();
    }

    // pre generate
    for (const key in this.#generators) {
      this.#generators[key]!.pre_gen();
    }

    // generate code
    for (const key in this.#generators) {
      this.#generators[key]!.gen();
    }

    // post generate
    for (const key in this.#generators) {
      this.#generators[key]!.post_gen();
    }
  }

  // step 6. output result
  output() {
    const out_dir = this.project_db.config.output_dir

    // output headers
    for (const header of this.project_db.main_module.headers) {
      const out_path = path.join(out_dir, header.output_header_path);
      this.#output_code(out_path, header.gen_code)
    }

    // output sources
    this.#output_code(path.join(out_dir, "generated.cpp"), this.project_db.main_module.gen_code);
  }
  #output_code(out_path: string, code: CodeBuilder) {
    const out_dir = path.dirname(out_path);

    // make dir
    if (!fs.existsSync(out_dir)) {
      fs.mkdirSync(out_dir, { recursive: true });
    }

    // write file
    fs.writeFileSync(out_path, code.content, { encoding: "utf-8" });
  }

  // generator apis
  add_generator(name: string, gen: Generator) {
    if (this.#generators[name]) {
      throw new Error(`Generator ${name} already exists`);
    }
    this.#generators[name] = gen;
    gen.owner = this;
  }
}
