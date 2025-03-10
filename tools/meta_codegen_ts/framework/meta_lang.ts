// deno-lint-ignore-file
import * as peggy from "peggy";
import * as fs from "node:fs";
import * as path from "node:path";

//======================== scheme ========================
type ValueType = string | number | boolean;
type ValueTypeStr = 'string' | 'number' | 'boolean';
type PresetFunc = () => void;
type ValueAssignFunc = (value: any) => void;
type ArrayAssignFunc = (value: any[], is_append: boolean) => void;
type CheckTypeType = { accept_type: ValueTypeStr; }
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
  if (typeof name != "string") throw new Error("name must be string");

  // check name conflict
  if (values[name]) throw new Error(`${String(meta_symbol)}[${name}] already exists`);

  // assign data
  values[name] = value;
}
export function preset(name?: string) {
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
export function value(accept_type: ValueTypeStr, options: { name?: string } = {}) {
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
export function array(accept_type: ValueTypeStr, options: { name?: string } = {}) {
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
export function value_proxy(accept_type: ValueTypeStr) {
  return (target: any, method_name: string, desc: TypedPropertyDescriptor<ValueAssignFunc>) => {
    const metadata = target[Symbol.metadata] ??= {};
    metadata[symbol_value_proxy] = {
      func: desc.value,
      accept_type: accept_type,
    } as ValueProxyData;
  }
}
export function array_proxy(accept_type: ValueTypeStr) {
  return (target: any, method_name: string, desc: TypedPropertyDescriptor<ArrayAssignFunc>) => {
    const metadata = target[Symbol.metadata] ??= {};
    metadata[symbol_array_proxy] = {
      func: desc.value,
      accept_type: accept_type,
    } as ArrayProxyData;;
  }
}

//======================== compiler ========================
export class Compiler {
  #parser: peggy.Parser;

  constructor(grammar: string) {
    this.#parser = peggy.generate(grammar);
  }

  compile(input: string): Program {
    try {
      const exprs = this.#parser.parse(input);
      return new Program(exprs);
    } catch (e: any) {
      e.location as peggy.Location;
      throw new Error(
        `failed to parse meta lang expression
${e}
source: ${input}
at line ${e.location.start.line} column ${e.location.start.column}`,
      );
    }
  }

  static #global_parser: Compiler | null = null;
  static load() {
    this.#global_parser ??= new Compiler(
      fs.readFileSync(
        path.join(__dirname, "meta_lang.peggy"),
        "utf-8",
      ),
    );
    return this.#global_parser;
  }
}

//======================== ast ========================
type LiteralNode = {
  type: 'literal';
  value: ValueType;
  location: peggy.LocationRange;
};
type IdentifierNode = {
  type: 'identifier';
  value: string;
  location: peggy.LocationRange;
};
type AccessExprNode = {
  type: 'access';
  value: IdentifierNode[];
  location: peggy.LocationRange;
}
type PresetNode = {
  type: 'preset';
  value: string;
  location: peggy.LocationRange;
}
type PresetExprNode = {
  type: 'preset_expr';
  value: PresetNode[];
  location: peggy.LocationRange;
}
type ArrayExprNode = {
  type: 'array_expr';
  value: LiteralNode[];
  location: peggy.LocationRange;
}
type OperatorNode = {
  type: 'operator';
  op: "=" | "+=";
  left: AccessExprNode;
  right: LiteralNode | PresetExprNode | ArrayExprNode;
  location: peggy.LocationRange;
}

//======================== program ========================
class AccessFailedError extends Error {
  key: string;
  obj: any;
  location: peggy.LocationRange;

  constructor(key: string, obj: any, location: peggy.LocationRange) {
    super(`failed to access ${key} in ${obj.constructor.name}
  at script: ${location.start.line}：${location.start.column}`)

    this.key = key;
    this.obj = obj;
    this.location = location;
  }
}
class TypeMismatchError extends Error {
  expected: string;
  actual: any;
  location: peggy.LocationRange;

  constructor(expected: string, actual: any, location: peggy.LocationRange) {
    super(`type mismatch, expected ${expected}, but got ${actual} (${typeof actual})
  at script: ${location.start.line}：${location.start.column}`);
    this.expected = expected;
    this.actual = actual;
    this.location = location;
  }
}
class PresetNotFoundError extends Error {
  preset: string;
  obj: any;
  location: peggy.LocationRange;

  constructor(preset: string, obj: any, location: peggy.LocationRange) {
    super(`preset ${preset} not found in ${obj.constructor.name}
  at script: ${location.start.line}：${location.start.column}`);
    this.preset = preset;
    this.obj = obj;
    this.location = location;
  }
}
export class Program {
  #exprs: OperatorNode[];

  constructor(exprs: OperatorNode[]) {
    this.#exprs = exprs;
  }

  exec(obj: any) {
    for (const expr of this.#exprs) {
      this.#exec_expr(obj, expr);
    }
  }
  #exec_expr(obj: any, expr: OperatorNode) {
    // process access
    let cur_obj = obj;
    let cur_key!: string;
    expr.left.value.forEach((ident, idx) => {
      cur_key = ident.value;
      if (idx === expr.left.value.length - 1) return;
      let next_obj = cur_obj[ident.value];
      if (next_obj === undefined) {
        throw new AccessFailedError(
          ident.value,
          cur_obj,
          expr.left.location,
        )
      }
      cur_obj = next_obj;
    })

    // process operator
    if (expr.right.type === 'literal' || expr.right.type === 'array_expr') {
      // get symbol, only literal assign will be trait as value
      const is_array = expr.right.type === 'array_expr' || expr.op === '+=';
      const access_sb = is_array ? symbol_array : symbol_value;
      const proxy_sb = is_array ? symbol_array_proxy : symbol_value_proxy;

      // find accessor
      const metadata = Object.getPrototypeOf(cur_obj)[Symbol.metadata];
      const value_data = metadata?.[access_sb]?.[cur_key];
      if (value_data !== undefined) {
        // assign field
        Program.#check_type(value_data, expr);
        Program.#do_assign(cur_obj, value_data, expr);
        return;
      } else {
        // assign by proxy
        const proxy_obj = cur_obj[cur_key];
        if (proxy_obj !== undefined) {
          const proxy_metadata = Object.getPrototypeOf(proxy_obj)[Symbol.metadata];
          const proxy_value_data = proxy_metadata?.[proxy_sb];
          if (proxy_value_data !== undefined) {
            proxy_value_data as ValueProxyData;
            Program.#check_type(proxy_value_data, expr);
            Program.#do_proxy(proxy_obj, proxy_value_data, expr);
            return;
          }
        }
      }

    } else if (expr.right.type === "preset_expr") {
      // find preset expr
      const preset_obj = cur_obj[cur_key];
      if (preset_obj !== undefined) {
        const metadata = Object.getPrototypeOf(preset_obj)[Symbol.metadata];
        const preset_data_map = metadata?.[symbol_preset];
        if (preset_data_map !== undefined) {
          // each preset and assign
          for (const preset_node of expr.right.value) {
            const preset_data = preset_data_map[preset_node.value];
            if (preset_data === undefined) {
              throw new PresetNotFoundError(
                preset_node.value,
                preset_obj,
                preset_node.location,
              );
            } else {
              preset_data.func.call(preset_obj);
            }
          }
          return;
        }
      }

    } else {
      throw new Error("should not reach here");
    }

    // failed in all access case
    throw new AccessFailedError(
      cur_key,
      cur_obj,
      expr.left.location,
    );
  }
  static #check_type(data: CheckTypeType, expr: OperatorNode) {
    if (expr.right.type === 'literal') {
      // if literal, check value
      if (data.accept_type !== typeof expr.right.value) {
        throw new TypeMismatchError(
          data.accept_type,
          expr.right.value,
          expr.right.location,
        );
      }
    } else if (expr.right.type === 'array_expr') {
      // if array, check items
      for (const item of expr.right.value) {
        if (data.accept_type !== typeof item.value) {
          throw new TypeMismatchError(
            data.accept_type,
            item.value,
            item.location,
          );
        }
      }
    } else {
      throw new Error('should not reach here');
    }
  }
  static #do_assign(obj: any, data: ValueData | ArrayData, expr: OperatorNode) {
    const is_append = expr.op === '+='
    if (expr.right.type === 'literal') {
      if (is_append) { // literal append
        if (typeof data.field_or_method === 'string') {
          obj[data.field_or_method].push(expr.right.value);
        } else {
          const func = data.field_or_method as ArrayAssignFunc;
          func.call(obj, [expr.right.value], true);
        }
      } else {  // literal assign
        if (typeof data.field_or_method === 'string') {
          obj[data.field_or_method] = expr.right.value;
        } else {
          const func = data.field_or_method as ValueAssignFunc;
          func.call(obj, expr.right.value);
        }
      }
    } else if (expr.right.type === 'array_expr') {
      const array_param = expr.right.value.map((item) => item.value)

      if (typeof data.field_or_method === 'string') {
        if (is_append) { // array append
          obj[data.field_or_method].push(...array_param);
        } else { // array assign
          obj[data.field_or_method] = array_param;
        }
      } else {
        const func = data.field_or_method as ArrayAssignFunc;
        func.call(obj, array_param, is_append);
      }
    } else {
      throw new Error('should not reach here');
    }
  }
  static #do_proxy(obj: any, data: ValueProxyData | ArrayProxyData, expr: OperatorNode) {
    const is_append = expr.op === '+='


    if (expr.right.type === 'literal') {
      const typed_data = data as ValueProxyData;
      typed_data.func.call(obj, expr.right.value);
    } else if (expr.right.type === 'array_expr') {
      const typed_data = data as ArrayProxyData;
      typed_data.func.call(obj, expr.right.value.map((item) => item.value), false);
    } else {
      throw new Error('should not reach here');
    }
  }
}

//======================== util objects ========================
export class WithEnable {
  @value('boolean')
  enable: boolean = false

  @preset('enable')
  preset_enable() {
    this.enable = true;
  }
  @preset('disable')
  preset_disable() {
    this.enable = false;
  }
}
