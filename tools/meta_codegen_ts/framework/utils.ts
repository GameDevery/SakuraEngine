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
        `cannot find match type for field ${
          String(field.name)
        } of type ${obj.constructor.name}, candidate types: ${
          field.types.join(", ")
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
