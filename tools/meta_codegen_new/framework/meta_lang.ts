import * as peggy from "peggy";
import * as fs from "node:fs";
import * as path from "node:path";

export type Value = {
  type: "value";
  value: string | number | boolean;
};
export type Array = {
  type: "array";
  value: Value[];
};
export type Preset = {
  type: "preset";
  value: string[];
};
export type Expr = {
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
    return new Program(this.#parser.parse(input));
  }
}

export class Program {
  exprs: Expr[];

  constructor(exprs: Expr[]) {
    this.exprs = exprs;
  }
}

export function load_parser(): Parser {
  return new Parser(
    fs.readFileSync(
      path.join(path.dirname(import.meta.path), "meta_lang.peggy"),
      "utf-8",
    ),
  );
}
