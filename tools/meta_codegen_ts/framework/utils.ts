// deno-lint-ignore-file

// serialize helpers
class _SerdeFieldData {
  name: string | symbol;
  types: (string | Function)[];
  constructor(name: string | symbol, types: (string | Function)[]) {
    this.name = name;
    this.types = types;
  }
}
class _SerdeMetaData {
  fields_info: _SerdeFieldData[] = [];
}
function _get_serde_metadata_and_check(
  target: any,
  ctx: ClassFieldDecoratorContext,
) {
  const metadata = ctx.metadata[serde_key] as _SerdeMetaData;
  if (metadata === undefined) {
    throw new Error(
      `@serialize must be used with @serializable on class ${target.constructor.name}`,
    );
  }
  return metadata;
}
const serde_key = Symbol("serde");

// serialize decorators
export function serializable() {
  return (target: any, ctx: ClassDecoratorContext) => {
    ctx.metadata[serde_key] = new _SerdeMetaData();
  };
}
export function serialize(
  types: (string | Function)[],
  name?: string | symbol,
) {
  return (target: any, ctx: ClassFieldDecoratorContext) => {
    const metadata = _get_serde_metadata_and_check(target, ctx);
    const field_name = name ?? ctx.name;
    metadata.fields_info.push(new _SerdeFieldData(field_name, types));
  };
}
export function from_json(obj: any, json_obj: any) {
  const metadata = obj.constructor.metadata[serde_key] as _SerdeMetaData;
  if (metadata === undefined) {
    throw new Error(
      `object or type ${obj.constructor.name} is not serializable`,
    );
  }

  for (const field of metadata.fields_info) {
    // get type of field
    let type: string | Function | undefined = undefined;
    for (const t of field.types) {
      if (typeof t === "string") {
        if (typeof obj[field.name] === t) {
          type = t;
          break;
        }
      } else if (typeof t === "function") {
        if (obj[field.name] instanceof t) {
          type = t;
          break;
        }
      }
    }
    if (type === undefined) {
      throw new Error(
        `cannot find match type for field ${String(field.name)
        } of type ${obj.constructor.name}, candidate types: ${field.types.join(", ")
        }`,
      );
    }

    // check json object type
    if (typeof type === "string") {
      if (typeof json_obj[field.name] !== type) {
        throw new Error(
          `json object field ${String(field.name)} is not of type ${type}`,
        );
      }
    } else {
      if (json_obj[field.name]! instanceof type) {
        throw new Error(
          `json object field ${String(field.name)} is not of type ${type.name}`,
        );
      }
    }

    // assign
    obj[field.name] = json_obj[field.name];
  }



}

// code builder
export class CodeBuilder {
  indent_unit: number = 4;
  #indent_stack: number[] = [];
  #indent: number = 0;
  #content: string = "";

  get content(): string { return this.#content; }

  // indent
  push_indent(count: number = 1): void {
    this.#indent_stack.push(count);
    this.#indent += count;
  }
  pop_indent(): void {
    if (this.#indent_stack.length == 0) {
      throw new Error("indent stack is empty");
    }
    this.#indent -= this.#indent_stack.pop()!;
  }

  // append content 
  block(code_block: string) : void {
    if (this.#indent == 0) {
      this.#content += code_block;
    } else {
      const lines = code_block.split("\n");
      const indent_str = " ".repeat(this.#indent * this.indent_unit);
      for (const line of lines) {
        this.#content += `${indent_str}${line}\n`;
      }
    }
  }
  line(code: string) : void {
    if (this.#indent == 0) {
      this.#content += code;
    } else {
      const indent_str = " ".repeat(this.#indent * this.indent_unit);
      this.#content += `${indent_str}${code}`;
    }
  }
  wrap(pre: string, callback: (builder: CodeBuilder) => void, post: string): void {
    this.block(pre);
    callback(this);
    this.block(post);
  }
  empty_line(count: number): void {
    this.#content += "\n".repeat(count);
  }
}
