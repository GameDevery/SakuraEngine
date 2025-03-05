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
  constructor(name: string, json_obj: any) {
    fill_name(this, name);

    // load info
    this.is_scoped = json_obj.isScoped;
    this.underlying_type = json_obj.underlying_type;
    this.comment = json_obj.comment;
    this.file_name = json_obj.fileName;
    this.line = json_obj.line;

    // load values
    this.values = {};
    for (name in json_obj.values) {
      this.values[name] = new EnumValue(name, this, json_obj.values[name]);
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
  constructor(name: string, parent: Enum, json_obj: any) {
    this.parent = parent;

    // parse name
    fill_name(this, name);

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
  constructor(name: string, json_object: any) {
    // parse name
    fill_name(this, name);

    // info
    this.bases = json_object.bases;
    this.comment = json_object.comment;
    this.file_name = json_object.fileName;
    this.line = json_object.line;

    // load fields
    this.fields = [];
    for (const name in json_object.fields) {
      this.fields.push(new Field(this, json_object.fields[name]));
    }

    // load methods
    this.methods = [];
    for (const name in json_object.methods) {
      this.methods.push(new Method(this, json_object.methods[name]));
    }
  }
}
export class Field {
  constructor(parent: Record, json_object: any) {
  }
}
export class Method {
  constructor(parent: Record, json_object: any) {
  }
}
export class Parameter {
  constructor(parent: Method, json_object: any) {
  }
}
export class Function {
  constructor(json_object: any) {
  }
}
