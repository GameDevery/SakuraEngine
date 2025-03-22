// deno-lint-ignore-file
import * as peggy from "peggy";
import * as fs from "node:fs";
import * as path from "node:path";
import type { MetaLangLocation } from "./utils";

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
type AccessListenerFunc = (name: string) => void;
const symbol_preset = Symbol("preset");
const symbol_value = Symbol("value");
const symbol_array = Symbol("array");
const symbol_value_proxy = Symbol("value_proxy");
const symbol_array_proxy = Symbol("array_proxy");
const symbol_access_listener = Symbol("access_listener");
const ml_global_metadata_tables = new Map<any, any>();
function metadata_dict_of(target: any) {
  if (!ml_global_metadata_tables.has(target)) {
    ml_global_metadata_tables.set(target, {});
  }
  return ml_global_metadata_tables.get(target);
}
function check_and_assign(target: any, meta_symbol: symbol, name: string, value: any) {
  // add metadata
  const metadata_dict = metadata_dict_of(target)
  metadata_dict[meta_symbol] ??= {};
  const values = metadata_dict[meta_symbol] as Dict<PresetData>;

  // solve name
  if (typeof name != "string") throw new Error("name must be string");

  // check name conflict
  if (values[name]) {
    throw new Error(`${String(meta_symbol)}[${name}] already exists`);
  }

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
    metadata_dict_of(target)[symbol_value_proxy] = {
      func: desc.value,
      accept_type: accept_type,
    } as ValueProxyData;
  }
}
export function array_proxy(accept_type: ValueTypeStr) {
  return (target: any, method_name: string, desc: TypedPropertyDescriptor<ArrayAssignFunc>) => {
    metadata_dict_of(target)[symbol_array_proxy] = {
      func: desc.value,
      accept_type: accept_type,
    } as ArrayProxyData;;
  }
}
export function access_listener() {
  return (target: any, method_name: string, desc: TypedPropertyDescriptor<AccessListenerFunc>) => {
    metadata_dict_of(target)[symbol_access_listener] = desc.value;
  }
}

//======================== compiler ========================
export class CompileError extends Error {
  constructor(
    public start: MetaLangLocation,
    public end: MetaLangLocation,
    public error: string,
  )
  {
    super(error);
  }
}
export class Compiler {
  #parser: peggy.Parser;

  constructor(grammar: string) {
    this.#parser = peggy.generate(grammar);
  }

  compile(input: string): Program {
    try {
      const exprs = this.#parser.parse(input);
      return new Program(exprs, input);
    } catch (e: any) {
      const expect_str = e.expected
        .map((item: any) => {
          if (item.type === 'literal') {
            return item.text;
          } else if (item.type === 'other') {
            return item.description;
          } else if (item.type === 'end') {
            return '[END]'
          } else {
            return undefined
          }
        })
        .filter((item: any) => item !== undefined)
        .join(", ");
      throw new CompileError(
        e.location.start,
        e.location.end,
        `expected ${expect_str}, but got ${e.found}`
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
export class AccessFailedError extends Error {
  key: string;
  obj: any;
  location: peggy.LocationRange;

  constructor(key: string, obj: any, location: peggy.LocationRange) {
    super(`failed to access ${key} in ${obj.constructor.name}`)

    this.key = key;
    this.obj = obj;
    this.location = location;
  }
}
export class TypeMismatchError extends Error {
  expected: string;
  actual: any;
  location: peggy.LocationRange;

  constructor(expected: string, actual: any, location: peggy.LocationRange) {
    super(`type mismatch`);
    this.expected = expected;
    this.actual = actual;
    this.location = location;
  }
}
export class PresetNotFoundError extends Error {
  preset: string;
  obj: any;
  location: peggy.LocationRange;

  constructor(preset: string, obj: any, location: peggy.LocationRange) {
    super(`preset not found`);
    this.preset = preset;
    this.obj = obj;
    this.location = location;
  }
}
export class Program {
  #exprs: OperatorNode[];
  source: string;

  constructor(exprs: OperatorNode[], source: string) {
    this.#exprs = exprs;
    this.source = source;
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
      // store access key
      cur_key = ident.value;

      // ignore last access
      if (idx === expr.left.value.length - 1) return;

      // try access
      let next_obj = cur_obj[cur_key];
      if (next_obj === undefined) {
        throw new AccessFailedError(
          cur_key,
          cur_obj,
          expr.left.location,
        )
      }

      Program.#invoke_access_listener(cur_obj, cur_key);

      // update current object
      cur_obj = next_obj;
    })

    // process operator
    if (expr.right.type === 'literal' || expr.right.type === 'array_expr') {
      // get symbol, only literal assign will be trait as value
      const is_array = expr.right.type === 'array_expr' || expr.op === '+=';
      const access_sb = is_array ? symbol_array : symbol_value;
      const proxy_sb = is_array ? symbol_array_proxy : symbol_value_proxy;

      // find accessor
      let value_data
      Program.#each_metadata(cur_obj, access_sb, (metadata) => {
        const found_value_data = metadata?.[cur_key];
        if (found_value_data !== undefined) {
          value_data = found_value_data
        }
      })
      if (value_data !== undefined) {
        // assign field
        Program.#check_type(value_data, expr);
        Program.#do_assign(cur_obj, value_data, expr);
        return;
      } else {
        // assign by proxy
        const proxy_obj = cur_obj[cur_key];
        if (proxy_obj !== undefined) {
          let success = false
          Program.#each_metadata(proxy_obj, proxy_sb, (proxy_value_data) => {
            proxy_value_data as ValueProxyData;
            Program.#check_type(proxy_value_data, expr);
            Program.#invoke_access_listener(cur_obj, cur_key);
            Program.#do_proxy(proxy_obj, proxy_value_data, expr);
            success = true
          })
          if (success) { return; }
        }
      }

    } else if (expr.right.type === "preset_expr") {
      // find preset expr
      const preset_obj = cur_obj[cur_key];
      if (preset_obj !== undefined) {
        const preset_data_map: Dict<any> = {}
        Program.#each_metadata(preset_obj, symbol_preset, (preset_data) => {
          for (const k in preset_data) {
            preset_data_map[k] = preset_data[k];
          }
        })
        Program.#invoke_access_listener(cur_obj, cur_key);
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
    Program.#invoke_access_listener(obj, data.name);

    // do assign or append
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
      if (is_append) { // literal append
        const typed_data = data as ArrayProxyData;
        typed_data.func.call(obj, [expr.right.value], true);
      } else { // literal assign
        const typed_data = data as ValueProxyData;
        typed_data.func.call(obj, expr.right.value);
      }
    } else if (expr.right.type === 'array_expr') {
      const typed_data = data as ArrayProxyData;
      typed_data.func.call(obj, expr.right.value.map((item) => item.value), is_append);
    } else {
      throw new Error('should not reach here');
    }
  }
  static #invoke_access_listener(obj: any, key: string) {
    Program.#each_metadata(obj, symbol_access_listener, (access_listener) => {
      access_listener as AccessListenerFunc
      access_listener.call(obj, key);
    })
  }
  static #each_metadata(obj: any, meta_symbol: symbol, each_func: (value: any) => void): any {
    let prototype = Object.getPrototypeOf(obj);

    while (prototype !== null) {
      // find metadata
      const metadata = metadata_dict_of(prototype)[meta_symbol];
      if (metadata !== undefined) {
        each_func(metadata);
      }

      prototype = Object.getPrototypeOf(prototype);
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

  // any access with `xxx.enable = true` effect
  @access_listener()
  ml_access(key: string) {
    this.enable = true;
  }
}
