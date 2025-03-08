// deno-lint-ignore-file
import * as ml from "./meta_lang.ts";

// help functions
type NameWithNamespace = {
  name: string;
  short_name: string;
  namespace: string[];
};
function fill_name(obj: NameWithNamespace, name: string) {
  obj.name = name;

  const split_name = name.split("::");
  obj.short_name = split_name[split_name.length - 1];
  obj.namespace = split_name.slice(0, split_name.length - 1);
}

export type AnyType =
  | Enum
  | EnumValue
  | Record
  | Field
  | Method
  | Function
  | Parameter;

// enum
export class Enum {
  // name & namespace
  name: string = "";
  short_name: string = "";
  namespace: string[] = [];

  // info
  is_scoped: boolean;
  underlying_type: string;
  comment: string;
  file_name: string;
  line: number;
  values: EnumValue[] = [];

  // attrs
  raw_attrs: string[] = [];
  attrs: ml.Program[] = [];
  gen_data: Dict<any> = {};

  // deno-lint-ignore no-explicit-any
  constructor(json_obj: any) {
    fill_name(this, json_obj.name);

    // load info
    this.is_scoped = json_obj.is_scoped;
    this.underlying_type = json_obj.underlying_type;
    this.comment = json_obj.comment;
    this.file_name = json_obj.file_name;
    this.line = json_obj.line;

    // load values
    for (const value of json_obj.values) {
      this.values.push(new EnumValue(this, value));
    }

    // load attrs
    this.raw_attrs = json_obj.attrs;
  }
}
export class EnumValue {
  // owner
  parent: Enum;

  // name & namespace
  name: string = "";
  short_name: string = "";
  namespace: string[] = [];

  // info
  value: number;
  comment: string;
  line: number;

  // attrs
  raw_attrs: string[] = [];
  attrs: ml.Program[] = [];
  gen_data: Dict<any> = {};

  // deno-lint-ignore no-explicit-any
  constructor(parent: Enum, json_obj: any) {
    this.parent = parent;

    // parse name
    fill_name(this, json_obj.name);

    // parse json
    this.value = json_obj.value;
    this.comment = json_obj.comment;
    this.line = json_obj.line;

    // load attrs
    this.raw_attrs = json_obj.attrs;
  }
}

// record
export class Record {
  // name & namespace
  name: string = "";
  short_name: string = "";
  namespace: string[] = [];

  // info
  bases: string[];
  fields: Field[];
  methods: Method[];
  file_name: string;
  line: number;
  comment: string;
  has_generate_body_flag: boolean = false;
  generate_body_line: number = 0;
  generate_body_content = "";

  // attrs
  raw_attrs: string[] = [];
  attrs: ml.Program[] = [];
  gen_data: Dict<any> = {};

  // deno-lint-ignore no-explicit-any
  constructor(json_obj: any) {
    // parse name
    fill_name(this, json_obj.name);

    // info
    this.bases = json_obj.bases;
    this.comment = json_obj.comment;
    this.file_name = json_obj.file_name;
    this.line = json_obj.line;

    // load fields
    this.fields = [];
    for (const field of json_obj.fields) {
      this.fields.push(new Field(this, field));
    }

    // load methods
    this.methods = [];
    for (const method of json_obj.methods) {
      if (method.name === "_zz_skr_generate_body_flag") {
        this.has_generate_body_flag = true;
        this.generate_body_line = method.line;
      } else {
        this.methods.push(new Method(this, method));
      }
    }

    // load attrs
    this.raw_attrs = json_obj.attrs;
  }

  dump_generate_body(): string {
    return this.generate_body_content.split("\n").join("\\\n");
  }
}
export class Field {
  parent: Record;

  name: string;

  type: string;
  raw_type: string;
  access: string;
  default_value: string;
  array_size: number;
  is_functor: boolean;
  is_static: boolean;
  is_anonymous: boolean;
  comment: string;
  line: number;

  // attrs
  raw_attrs: string[] = [];
  attrs: ml.Program[] = [];
  gen_data: Dict<any> = {};

  // deno-lint-ignore no-explicit-any
  constructor(parent: Record, json_obj: any) {
    this.parent = parent;

    this.name = json_obj.name;

    this.type = json_obj.type;
    this.raw_type = json_obj.raw_type;
    this.access = json_obj.access;
    this.default_value = json_obj.default_value;
    this.array_size = json_obj.array_size;
    this.is_functor = json_obj.is_functor;
    this.is_static = json_obj.is_static;
    this.is_anonymous = json_obj.is_anonymous;
    this.comment = json_obj.comment;
    this.line = json_obj.line;

    // load attrs
    this.raw_attrs = json_obj.attrs;
  }
}
export class Method {
  parent: Record;

  name: string = "";
  short_name: string = "";
  namespace: string[] = [];

  access: string;
  is_static: boolean;
  is_const: boolean;
  is_nothrow: boolean;
  comment: string;
  parameters: Parameter[] = [];
  ret_type: string;
  raw_ret_type: string;
  line: string;

  // attrs
  raw_attrs: string[] = [];
  attrs: ml.Program[] = [];
  gen_data: Dict<any> = {};

  // deno-lint-ignore no-explicit-any
  constructor(parent: Record, json_obj: any) {
    this.parent = parent;

    fill_name(this, json_obj.name);

    this.access = json_obj.access;
    this.is_static = json_obj.is_static;
    this.is_const = json_obj.is_const;
    this.is_nothrow = json_obj.is_nothrow;
    this.comment = json_obj.comment;

    // load parameters
    for (const parameter of json_obj.parameters) {
      this.parameters.push(new Parameter(this, parameter));
    }

    // load return type
    this.ret_type = json_obj.ret_type;
    this.raw_ret_type = json_obj.raw_ret_type;
    this.line = json_obj.line;

    // load attrs
    this.raw_attrs = json_obj.attrs;
  }

  dump_params() {
    return this.parameters.map(param=> `${param.type} ${param.name}`).join(", ")
  }
  dump_params_with_comma() {
    const params = this.dump_params();
    return params.length > 0 ? `, ${params}` : "";
  }
  dump_params_name_only() {
    return this.parameters.map(param=> param.name).join(", ")
  }
  dump_params_name_only_with_comma() {
    const params = this.dump_params_name_only();
    return params.length > 0 ? `, ${params}` : "";
  }
  dump_const() {
    return this.is_const ? "const " : "";
  }
  dump_noexcept() {
    return this.is_nothrow ? " noexcept" : "";
  }
}
export class Parameter {
  parent: Method | Function;

  name: string;
  type: string;
  array_size: number;
  raw_type: string;
  is_functor: boolean;
  is_callback: boolean;
  is_anonymous: boolean;
  default_value: string;
  comment: string;
  line: number;

  // attrs
  raw_attrs: string[] = [];
  attrs: ml.Program[] = [];
  gen_data: Dict<any> = {};

  // deno-lint-ignore no-explicit-any
  constructor(parent: Method | Function, json_obj: any) {
    this.parent = parent;

    this.name = json_obj.name;
    this.type = json_obj.type;
    this.array_size = json_obj.array_size;
    this.raw_type = json_obj.raw_type;
    this.is_functor = json_obj.is_functor;
    this.is_callback = json_obj.is_callback;
    this.is_anonymous = json_obj.is_anonymous;
    this.default_value = json_obj.default_value;
    this.comment = json_obj.comment;
    this.line = json_obj.line;

    // load attrs
    this.raw_attrs = json_obj.attrs;
  }
}
export class Function {
  name: string = "";
  short_name: string = "";
  namespace: string[] = [];

  is_static: boolean;
  is_const: boolean;
  comment: string;
  parameters: Parameter[] = [];
  ret_type: string;
  raw_ret_type: string;
  file_name: string;
  line: number;

  // attrs
  raw_attrs: string[] = [];
  attrs: ml.Program[] = [];
  gen_data: Dict<any> = {};

  // deno-lint-ignore no-explicit-any
  constructor(json_obj: any) {
    fill_name(this, json_obj.name);

    this.is_static = json_obj.is_static;
    this.is_const = json_obj.is_const;
    this.comment = json_obj.comment;

    // load parameters
    for (const parameter of json_obj.parameters) {
      this.parameters.push(new Parameter(this, parameter));
    }

    // load return type
    this.ret_type = json_obj.ret_type;
    this.raw_ret_type = json_obj.raw_ret_type;
    this.file_name = json_obj.file_name;
    this.line = json_obj.line;

    // load attrs
    this.raw_attrs = json_obj.attrs;
  }
}
