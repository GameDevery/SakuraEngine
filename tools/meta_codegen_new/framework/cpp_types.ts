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
  values: { [key: string]: EnumValue };

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
    this.values = {};
    for (const value of json_obj.values) {
      this.values[name] = new EnumValue(this, value);
    }
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

  // deno-lint-ignore no-explicit-any
  constructor(parent: Enum, json_obj: any) {
    this.parent = parent;

    // parse name
    fill_name(this, json_obj.name);

    // parse json
    this.value = json_obj.value;
    this.comment = json_obj.comment;
    this.line = json_obj.line;
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

  // deno-lint-ignore no-explicit-any
  constructor(json_object: any) {
    // parse name
    fill_name(this, json_object.name);

    // info
    this.bases = json_object.bases;
    this.comment = json_object.comment;
    this.file_name = json_object.file_name;
    this.line = json_object.line;

    // load fields
    this.fields = [];
    for (const field of json_object.fields) {
      this.fields.push(new Field(this, field));
    }

    // load methods
    this.methods = [];
    for (const method of json_object.methods) {
      if (method.name === "_zz_skr_generate_body_flag") {
        this.has_generate_body_flag = true;
        this.generate_body_line = method.line;
      } else {
        this.methods.push(new Method(this, method));
      }
    }
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

  // deno-lint-ignore no-explicit-any
  constructor(parent: Record, json_object: any) {
    this.parent = parent;

    this.name = json_object.name;

    this.type = json_object.type;
    this.raw_type = json_object.raw_type;
    this.access = json_object.access;
    this.default_value = json_object.default_value;
    this.array_size = json_object.array_size;
    this.is_functor = json_object.is_functor;
    this.is_static = json_object.is_static;
    this.is_anonymous = json_object.is_anonymous;
    this.comment = json_object.comment;
    this.line = json_object.line;
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
  parameters: { [key: string]: Parameter };
  ret_type: string;
  raw_ret_type: string;
  line: string;

  // deno-lint-ignore no-explicit-any
  constructor(parent: Record, json_object: any) {
    this.parent = parent;

    fill_name(this, json_object.name);

    this.access = json_object.access;
    this.is_static = json_object.is_static;
    this.is_const = json_object.is_const;
    this.is_nothrow = json_object.is_nothrow;
    this.comment = json_object.comment;

    // load parameters
    this.parameters = {};
    for (const parameter of json_object.parameters) {
      this.parameters[parameter.name] = new Parameter(this, parameter);
    }

    // load return type
    this.ret_type = json_object.ret_type;
    this.raw_ret_type = json_object.raw_ret_type;
    this.line = json_object.line;
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

  // deno-lint-ignore no-explicit-any
  constructor(parent: Method | Function, json_object: any) {
    this.parent = parent;

    this.name = json_object.name;
    this.type = json_object.type;
    this.array_size = json_object.array_size;
    this.raw_type = json_object.raw_type;
    this.is_functor = json_object.is_functor;
    this.is_callback = json_object.is_callback;
    this.is_anonymous = json_object.is_anonymous;
    this.default_value = json_object.default_value;
    this.comment = json_object.comment;
    this.line = json_object.line;
  }
}
export class Function {
  name: string = "";
  short_name: string = "";
  namespace: string[] = [];

  is_static: boolean;
  is_const: boolean;
  comment: string;
  parameters: { [key: string]: Parameter };
  ret_type: string;
  raw_ret_type: string;
  file_name: string;
  line: number;

  // deno-lint-ignore no-explicit-any
  constructor(json_object: any) {
    fill_name(this, json_object.name);

    this.is_static = json_object.is_static;
    this.is_const = json_object.is_const;
    this.comment = json_object.comment;

    // load parameters
    this.parameters = {};
    for (const parameter of json_object.parameters) {
      this.parameters[parameter.name] = new Parameter(this, parameter);
    }

    // load return type
    this.ret_type = json_object.ret_type;
    this.raw_ret_type = json_object.raw_ret_type;
    this.file_name = json_object.file_name;
    this.line = json_object.line;
  }
}
