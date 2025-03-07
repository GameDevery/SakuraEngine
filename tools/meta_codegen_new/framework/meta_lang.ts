// deno-lint-ignore-file
import * as peggy from "peggy";
import * as fs from "node:fs";
import * as path from "node:path";

// meta lang parse result
type Value = {
  type: "value";
  value: string | number | boolean;
};
type Array = {
  type: "array";
  value: (string | number | boolean)[];
};
type Preset = {
  type: "preset";
  value: string[];
};
type Expr = {
  visitor: string[];
  op: "=" | "+=";
  value: Value | Array | Preset;
};

export class Parser {
  #parser: peggy.Parser;

  constructor(grammar: string) {
    this.#parser = peggy.generate(grammar);
  }

  parse(input: string): Program {
    try {
      const exprs = this.#parser.parse(input);
      return new Program(exprs);
    } catch (e: any) {
      throw new Error(
        `failed to parse meta lang expression
${e}
source: ${input}
at line ${e.location.start.line} column ${e.location.start.column}`,
      );
    }
  }
}

export class Program {
  exprs: Expr[];

  constructor(exprs: Expr[]) {
    this.exprs = exprs;
  }

  exec(obj: any, ctx: ExecContext) {
    for (const expr of this.exprs) {
      if (expr.op === "=") {
        if (expr.value.type === "value" || expr.value.type === "array") {
          Program.assign_value(obj, expr.visitor, expr.value);
        } else if (expr.value.type === "preset") {
          const visitor_key = expr.visitor.join(".");
          for (const preset of expr.value.value) {
            ctx.presets[visitor_key](preset, obj, PresetTools);
          }
        } else {
          throw new Error(
            `not support value ${expr.value} in meta lang`,
          );
        }
      } else if (expr.op === "+=") {
        if (expr.value.type === "value" || expr.value.type === "array") {
          Program.append_value(obj, expr.visitor, expr.value);
        } else if (expr.value.type === "preset") {
          throw new Error(
            `preset not support "+=" operator, visitor: ${
              expr.visitor.join(
                ".",
              )
            }`,
          );
        } else {
          throw new Error(
            `not support value ${expr.value} in meta lang`,
          );
        }
      }
    }
  }

  static assign_value(obj: any, keys: string[], value: Value | Array) {
    let cur_obj = obj;
    keys.forEach((k, i) => {
      if (i == keys.length - 1) {
        // assign value
        cur_obj[k] = value.value;
      } else {
        // create object
        if (cur_obj[k] == undefined) {
          cur_obj[k] = {};
        }

        // check type
        if (typeof cur_obj[k] != "object") {
          throw new Error(
            `type conflict, old value is ${
              cur_obj[k]
            }, excepted object, when assign "${
              keys.slice(0, i + 1).join(".")
            }"`,
          );
        }

        // update cur_obj
        cur_obj = cur_obj[k];
      }
    });
  }
  static append_value(obj: any, keys: string[], value: Value | Array) {
    let cur_obj = obj;
    keys.forEach((k, i) => {
      if (i == keys.length - 1) {
        // create array
        if (cur_obj[k] == undefined) {
          cur_obj[k] = [];
        }

        // check type
        if (!Array.isArray(cur_obj[k])) {
          throw new Error(
            `type conflict, old value is ${
              cur_obj[k]
            }, excepted array, when assign "${keys.join(".")}"`,
          );
        }

        // append value
        if (value.type == "value") {
          cur_obj[k].push(value.value);
        } else if (value.type == "array") {
          cur_obj[k].push(...value.value);
        } else {
          throw new Error(
            `not support value ${value} for array append`,
          );
        }
      } else {
        // create object
        if (cur_obj[k] == undefined) {
          cur_obj[k] = {};
        }

        // check type
        if (typeof cur_obj[k] != "object") {
          throw new Error(
            `type conflict, old value is ${
              cur_obj[k]
            }, excepted object, when assign "${
              keys.slice(0, i + 1).join(".")
            }"`,
          );
        }

        // update cur_obj
        cur_obj = cur_obj[k];
      }
    });
  }
}

export function load_parser(): Parser {
  return new Parser(
    fs.readFileSync(
      path.join(__dirname, "meta_lang.peggy"),
      "utf-8",
    ),
  );
}

type PresetToolType = {
  assign(obj: any, key: string, value: any): void;
  append(obj: any, key: string, value: any): void;
};
export type PresetCallback = (
  name: string,
  obj: any,
  tool: PresetToolType,
) => void;
class PresetTools {
  static assign(obj: any, key: string, value: any) {
    const keys = key.split(".");
    const value_obj: Value = {
      type: "value",
      value: value,
    };
    Program.assign_value(obj, keys, value_obj);
  }
  static append(obj: any, key: string, value: any) {
    const keys = key.split(".");
    let value_obj: Value | Array;
    if (Array.isArray(value)) {
      value_obj = {
        type: "array",
        value: value,
      };
    } else {
      value_obj = {
        type: "value",
        value: value,
      };
    }
    Program.append_value(obj, keys, value_obj);
  }
}
export class ExecContext {
  presets: { [key: string]: PresetCallback } = {};
}
