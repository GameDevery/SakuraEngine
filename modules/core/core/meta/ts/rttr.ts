import * as gen from "@framework/generator"
import * as db from "@framework/database"
import * as cpp from "@framework/cpp_types"

class ParamData {
  flags: string[] = []
  attrs: string[] = []
}

class MethodData {
  enable: boolean = false
  getter: string = ""
  setter: string = ""
}

class FieldData {
  enable: boolean = false
  getter: string = ""
  setter: string = ""
}

class RecordData {
  enable: boolean = false
  reflect_bases: string[] = []
  exclude_bases: string[] = []
  reflect_fields: boolean = false
  reflect_methods: boolean = false
  flags: string[] = []
  attrs: string[] = []
}

class EnumValueData {
  enable: boolean = false
  flags: string[] = []
  attrs: string[] = []
}

class EnumData {
  enable: boolean = false
  flags: string[] = []
  attrs: string[] = []
}

class RttrGenerator extends gen.Generator {
}

export function load_generator(gm: gen.GenerateManager) {
  gm.add_generator("rttr", new RttrGenerator())
}