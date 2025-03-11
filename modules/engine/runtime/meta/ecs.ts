import * as gen from "@framework/generator";
import * as db from "@framework/database";
import * as ml from "@framework/meta_lang";
import { CodeBuilder } from "@framework/utils";


class ECSGenerator extends gen.Generator {

}


export function load_generator(gm: gen.GenerateManager) {
  gm.add_generator("ecs", new ECSGenerator());
}
