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
            `preset not support "+=" operator, visitor: ${expr.visitor.join(
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
            `type conflict, old value is ${cur_obj[k]
            }, excepted object, when assign "${keys.slice(0, i + 1).join(".")
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
            `type conflict, old value is ${cur_obj[k]
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
            `type conflict, old value is ${cur_obj[k]
            }, excepted object, when assign "${keys.slice(0, i + 1).join(".")
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

//======================== scheme ========================
type ValueType = string | number | boolean;
type ValueTypeStr = 'string' | 'number' | 'boolean';
type PresetFunc = () => void;
type ValueAssignFunc = (value: any) => void;
type ArrayAssignFunc = (value: any[]) => void;
type PresetData = {
  name: string;
  func: PresetFunc;
}
type ValueData = {
  name: string;
  accept_type: ValueTypeStr;
  field_or_method: string | ValueAssignFunc;
}
type ArrayData = {
  name: string;
  accept_type: ValueTypeStr;
  field_or_method: string | ArrayAssignFunc;
}
type ValueProxyData = {
  accept_type: ValueTypeStr;
  func: ValueAssignFunc;
}
type ArrayProxyData = {
  accept_type: ValueTypeStr;
  func: ArrayAssignFunc;
}
const symbol_preset = Symbol("preset");
const symbol_value = Symbol("value");
const symbol_array = Symbol("array");
const symbol_value_proxy = Symbol("value_proxy");
const symbol_array_proxy = Symbol("array_proxy");
function check_and_assign(target: any, meta_symbol: symbol, name: string, value: any) {
  // add metadata
  const metadata = target[Symbol.metadata] ??= {};
  metadata[meta_symbol] ??= {};
  const values = metadata[meta_symbol] as Dict<PresetData>;

  // solve name
  if (typeof name != "string") throw new Error("preset name must be string");

  // check name conflict
  if (values[name]) throw new Error(`${String(meta_symbol)}[${name}] already exists`);

  // assign data
  values[name] = value;
}
export function ml_preset(name?: string) {
  return (target: any, method_name: string, desc: TypedPropertyDescriptor<PresetFunc>) => {
    check_and_assign(
      target,
      symbol_preset,
      name ?? method_name,
      {
        name: name ?? method_name,
        func: desc.value,
      } as PresetData
    )
  }
}
export function ml_value(accept_type: ValueTypeStr, options: { name?: string } = {}) {
  return (target: any, field_or_method: string, desc?: TypedPropertyDescriptor<ValueAssignFunc>) => {
    if (desc !== undefined) {
      check_and_assign(
        target,
        symbol_value,
        options.name ?? field_or_method,
        {
          name: options.name ?? field_or_method,
          accept_type: accept_type,
          field_or_method: desc.value,
        } as ValueData
      );
    } else {
      check_and_assign(
        target,
        symbol_value,
        options.name ?? field_or_method,
        {
          name: options.name ?? field_or_method,
          accept_type: accept_type,
          field_or_method: field_or_method,
        } as ValueData
      );
    }
  }
}
export function ml_array(accept_type: ValueTypeStr, options: { name?: string } = {}) {
  return (target: any, field_or_method: string, desc?: TypedPropertyDescriptor<ArrayAssignFunc>) => {
    if (desc !== undefined) {
      check_and_assign(
        target,
        symbol_array,
        options.name ?? field_or_method,
        {
          name: options.name ?? field_or_method,
          accept_type: accept_type,
          field_or_method: desc.value,
        } as ArrayData
      );
    } else {
      check_and_assign(
        target,
        symbol_array,
        options.name ?? field_or_method,
        {
          name: options.name ?? field_or_method,
          accept_type: accept_type,
          field_or_method: field_or_method,
        } as ArrayData
      );
    }
  }
}
export function ml_value_proxy(accept_type: ValueTypeStr) {
  return (target: any, method_name: string, desc: TypedPropertyDescriptor<ValueAssignFunc>) => {
    check_and_assign(
      target,
      symbol_value_proxy,
      method_name,
      {
        accept_type: accept_type,
        func: desc.value,
      } as ValueProxyData
    )
  }
}
export function ml_array_proxy(accept_type: ValueTypeStr) {
  return (target: any, method_name: string, desc: TypedPropertyDescriptor<ArrayAssignFunc>) => {
    check_and_assign(
      target,
      symbol_array_proxy,
      method_name,
      {
        accept_type: accept_type,
        func: desc.value,
      } as ArrayProxyData
    )
  }
}