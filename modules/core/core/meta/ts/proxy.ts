import * as gen from "@framework/generator"
import * as db from "@framework/database"
import * as cpp from "@framework/cpp_types"

class RecordData {
  enable: boolean = false
}

class MethodData {
  enable: boolean = false
  getter: string = ""
  setter: string = ""
}


class ProxyGenerator extends gen.Generator {
}

export function load_generator(gm: gen.GenerateManager) {
  gm.add_generator("proxy", new ProxyGenerator())
}