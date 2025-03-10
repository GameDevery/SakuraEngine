import * as gen from "@framework/generator"
import * as db from "@framework/database"
import * as ml from "@framework/meta_lang"

class RecordData {
  enable: boolean = false
  json: boolean = false
  bin: boolean = false
  field_default: string[] = []
}

class FieldData {
  enable: boolean = false
  json: boolean = false
  bin: boolean = false
  alias: string = ""
}

class EnumData {
  enable: boolean = false
  json: boolean = false
  bin: boolean = false
}

class SerializeGenerator extends gen.Generator {

}

export function load_generator(gm: gen.GenerateManager) {
  gm.add_generator("serialize", new SerializeGenerator());
}