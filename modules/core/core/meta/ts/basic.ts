import * as gen from "@framework/generator"
import * as db from "@framework/database"

class BasicGenerator extends gen.Generator {
}

export function load_generator(gm: gen.GenerateManager) {
  gm.add_generator("basic", new BasicGenerator())
}