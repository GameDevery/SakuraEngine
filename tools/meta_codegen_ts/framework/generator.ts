import * as db from "./database.ts"

// TODO. 重组生成步骤
//   1. load database
//   2. inject configs
//   3. run meta-lang
//   4. generate code

export class Generator {
  owner!: GenerateManager
  get main_module_db(): db.Module { return this.owner.project_db.main_module; }
  get project_db(): db.Project { return this.owner.project_db; }

  // load attr writer

  // generate functions
  // TODO. 只一个 gen 函数，通过多个 CodeBuilder 来实现分区生成
  gen_body() { }
  pre_gen() { }
  gen() { }
  post_gen() { }
}

export class GenerateManager {
  project_db!: db.Project
  #generators: Dict<Generator> = {}

  // generator apis
  add_generator(name: string, gen: Generator) {
    if (this.#generators[name]) {
      throw new Error(`Generator ${name} already exists`)
    }
    this.#generators[name] = gen
    gen.owner = this
  }
}
