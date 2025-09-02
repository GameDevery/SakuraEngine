// deno-lint-ignore-file

// code builder
export class CodeBuilder {
  indent_unit: number = 4;
  #indent_stack: number[] = [];
  #indent: number = 0;
  #content: string = "";

  get content(): string { return this.#content; }
  get content_marco(): string { return this.#content.replace(/(\r\n|\n|\r)/g, "\\\n"); }

  // utils
  is_empty(): boolean { return this.#content.length == 0; }

  // indent
  #push_indent(count: number = 1): void {
    this.#indent_stack.push(count);
    this.#indent += count;
  }
  #pop_indent(): void {
    if (this.#indent_stack.length == 0) {
      throw new Error("indent stack is empty");
    }
    this.#indent -= this.#indent_stack.pop()!;
  }

  // codegen tools
  $line(code: string): void {
    if (this.#indent == 0) {
      this.#content += `${code}\n`;
    } else {
      const indent_str = " ".repeat(this.#indent * this.indent_unit);
      this.#content += `${indent_str}${code}\n`;
    }
  }
  $struct(name: string, content: (b: CodeBuilder) => void): void {
    this.$line(`struct ${name} {`);
    this.#push_indent(1);
    content(this);
    this.#pop_indent();
    this.$line(`};`);
  }
  $function(label: string, content: (b: CodeBuilder) => void): void {
    this.$line(`${label} {`)
    this.#push_indent();
    content(this);
    this.#pop_indent();
    this.$line(`}`)
  }
  $switch(value: string, content: (b: CodeBuilder) => void): void {
    this.$line(`switch (${value}) {`)
    this.#push_indent();
    content(this)
    this.#pop_indent();
    this.$line(`}`)
  }
  $if(...pairs: [string, (b: CodeBuilder) => void][]): void {
    pairs.forEach(([cond, block], idx) => {
      // if label
      if (idx === 0) {
        this.$line(`if (${cond}) {`);
      } else {
        if (cond.length > 0) {
          this.$line(`} else if (${cond}) {`);
        } else {
          if (idx !== pairs.length - 1) { throw new Error("else must at last block"); }
          this.$line(`} else {`);
        }
      }

      // block content
      this.#push_indent();
      block(this);
      this.#pop_indent();

      // last
      if (idx === pairs.length - 1) {
        this.$line(`}`);
      }
    })
  }
  $scope(content: (b: CodeBuilder) => void) {
    this.$line(`{`);
    this.#push_indent();
    content(this);
    this.#pop_indent();
    this.$line(`}`);
  }
  $namespace(name: string, content: (b: CodeBuilder) => void): void {
    if (name.length > 0) {
      this.$line(`namespace ${name} {`);
    }
    content(this);
    if (name.length > 0) {
      this.$line(`}`);
    }
  }
  $namespace_line(name: string, content: () => string): void {
    if (name.length > 0) {
      this.$line(`namespace ${name} { ${content()} }`);
    } else {
      this.$line(`${content()}`);
    }
  }
  $indent(content: (b: CodeBuilder) => void): void {
    this.#push_indent();
    content(this);
    this.#pop_indent();
  }
  $indent_pop(content: (b: CodeBuilder) => void): void {
    this.#pop_indent();
    content(this);
    this.#push_indent();
  }
  $generate_note() {
    this.$line(`//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!`)
    this.$line(`//!! THIS FILE IS GENERATED, ANY CHANGES WILL BE LOST !!`)
    this.$line(`//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!`)
    this.$line(``)
  }


  // to string
  toString(): string {
    return this.#content;
  }
}

// log shading
export class LogColor {
  static red(str: string) {
    return `\x1b[31m${str}\x1b[0m`;
  }
  static green(str: string) {
    return `\x1b[32m${str}\x1b[0m`;
  }
  static yellow(str: string) {
    return `\x1b[33m${str}\x1b[0m`;
  }
  static blue(str: string) {
    return `\x1b[34m${str}\x1b[0m`;
  }
  static magenta(str: string) {
    return `\x1b[35m${str}\x1b[0m`;
  }
  static cyan(str: string) {
    return `\x1b[36m${str}\x1b[0m`;
  }
  static white(str: string) {
    return `\x1b[37m${str}\x1b[0m`;
  }
  static gray(str: string) {
    return `\x1b[30m${str}\x1b[0m`;
  }
}

// error
export interface MetaLangLocation {
  line: number;
  column: number;
  offset: number;
}
export class MetaLangError {
  constructor(
    public cpp_location: string,
    public error_message: string,
    public source: string,
    public start: MetaLangLocation,
    public end: MetaLangLocation,
  ) {
  }

  async write_log() {
    await Bun.write(Bun.stderr, this.error_body());
  }

  error_body(): string {
    let result = "";

    // error message
    const colored_error = LogColor.red("error:");
    result += `${this.cpp_location}: ${colored_error} ${this.error_message}\n`;

    // source code
    result += this.#source_preview();

    return result;
  }


  #source_preview(): string {
    const lines = this.source.split("\n");
    let result = "";

    // print prev line
    for (let i = 0; i < this.start.line - 1; ++i) {
      result += this.#line(i + 1, lines[i]);
    }

    // print error lines
    for (let i = this.start.line - 1; i < this.end.line; ++i) {
      const line_content = lines[i];
      const error_start = i === this.start.line - 1 ? this.start.column - 1 : 0;
      const error_end = i === this.end.line - 1 ? this.end.column - 1 : line_content.length;
      result += this.#line(i + 1, line_content);
      result += this.#error_indicator(error_start, error_end);
    }

    // print last line
    for (let i = this.end.line; i < lines.length; ++i) {
      result += this.#line(i + 1, lines[i]);
    }

    return result;
  }
  #line_number(line: number): string {
    return `${line}`.padEnd(4, " ");
  }
  #line(line_number: number, content: string): string {
    return `${this.#line_number(line_number)}|${content}\n`;
  }
  #error_indicator(start: number, end: number): string {
    let indicator_count = end - start;
    let empty_space_count = start;
    if (indicator_count <= 0) {
      // empty_space_count += 1;
      indicator_count = 1;
    }
    const indicator = LogColor.green(" ".repeat(empty_space_count) + "^".repeat(indicator_count));
    return `${' '.repeat(4)}|${indicator}\n`;
  }
}