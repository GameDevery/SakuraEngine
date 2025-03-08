import * as db from "./database.ts"

export class Generator {
  owner!: GenerateManager

  // load attr writer

  // generate functions
  gen_body() {}
  pre_gen() {}
  gen() {}
  post_gen() { }
}

export class GenerateManager {
  project_db!: db.Project
  #generators: Dict<Generator> = {}
  #file_cache: Dict<string> = {}

  // generator apis
  add_generator(name: string, gen: Generator) {
    if (this.#generators[name]) {
      throw new Error(`Generator ${name} already exists`)
    }
    this.#generators[name] = gen
    gen.owner = this
  }
}
